/**
* Copyright (c) 2006-2026 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#include "Filewatcher.h"
#include "event/sdl/Event.h"
#include "common/Optional.h"
#include "common/StringMap.h"

#ifdef LOVE_LINUX
#include "filesystem"

#include "iostream"
#include "cstring"
#include "cerrno"

#include "sys/inotify.h"
#include "unistd.h"
#endif

#define instanceEvent() (Module::getInstance<event::Event>(Module::M_EVENT))

// LOVE

namespace love
{
namespace filesystem
{

Filewatcher::Filewatcher()
{
}

Filewatcher::~Filewatcher()
{
	tryClose();
}

void Filewatcher::tryClose() const
{
#ifdef LOVE_LINUX
	if (fd != -1)
		close(fd);
#endif
}

STRINGMAP_BEGIN(Filewatcher::EventType, Filewatcher::EVENT_ENUM_MAX, fileEventType)
{
	{ "created", Filewatcher::EVENT_CREATED },
	{ "modified", Filewatcher::EVENT_MODIFIED },
	{ "deleted", Filewatcher::EVENT_DELETED },
	{ "movedfrom", Filewatcher::EVENT_MOVED_FROM },
	{ "movedto", Filewatcher::EVENT_MOVED_TO },
}
STRINGMAP_END(Filewatcher::EventType, Filewatcher::EVENT_ENUM_MAX, fileEventType)

void Filewatcher::dumpEvents()
{
	auto eventmodule = instanceEvent();

	std::vector<Variant> vargs;
	vargs.reserve(2);

	for (const Filewatcher::FileEvent& event : events) {
		vargs.clear();

		const char *name = "unknow";
		getConstant(event.Type, name);

		vargs.push_back(Variant(name, std::strlen(name)));
		vargs.push_back(Variant(event.Path.c_str(), event.Path.length()));

		StrongRef<event::Message> msg(new event::Message("filechanged", vargs), Acquire::NORETAIN);
		eventmodule->push(msg);
	}

	events.clear();
}

void Filewatcher::pump()
{
#ifdef LOVE_LINUX
	if (fd == -1)
		return;

	alignas(inotify_event) char buffer[4096];

	while (true)
	{
		ssize_t length = read(fd, buffer, sizeof(buffer));

		if (length == -1)
		{
			if (errno == EINTR)
			    continue;

			break;
			// if (errno == EAGAIN || errno == EWOULDBLOCK)
		    // break;

			// std::cerr << "inotify read: "
				// << strerror(errno) << '\n';
		}

		if (length == 0)
			break;

		for (char* ptr = buffer; ptr < buffer + length;)
		{
			Optional<EventType> type;
			Optional<std::string> path;
			// EventType type;
			// const char *path;

			const auto* event = reinterpret_cast<inotify_event*>(ptr);

			switch (event->mask)
			{
				case IN_CREATE: type.set(EVENT_CREATED); break;
				case IN_MODIFY: type.set(EVENT_MODIFIED); break;
				case IN_DELETE: type.set(EVENT_DELETED); break;
				case IN_MOVED_FROM: type.set(EVENT_MOVED_FROM); break;
			}

			if (event->len > 0) {
				auto subpath = watchers[event->wd];
				path.set(subpath + "/" + event->name);
			}
				// path = event->name;

			if (type.hasValue && path.hasValue)
				events.push_back(FileEvent{type.value, path.value});

			ptr += sizeof(inotify_event) + event->len;
		}
	}
#endif

	dumpEvents();
}

void Filewatcher::watchFile(const std::string &path)
{
	tryClose();
#ifdef LOVE_LINUX
	fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

	if (fd == -1)
	{
		printf("cant initiated fd\n");
		return;
	}

	std::vector<std::string> watchPaths;
	watchPaths.push_back(path);

	try
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
		{
			if (entry.is_directory())
				watchPaths.push_back(entry.path().string());
		}
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		printf("LOVE - Warning: %s\n", e.what());
	}

	for (const auto& _path : watchPaths)
	{
		printf("%s\n", _path.c_str());
		int wd = inotify_add_watch(fd, _path.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY);
		if (wd == -1)
		{
			printf("cant initiated watcher in %s\n", _path.c_str());
			continue;
		}

		watchers[wd] = _path;
	}

#endif

 	printf("log files started in %s\n", path.c_str());
}

}
}