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

#pragma once

#include "common/config.h"

#include "map"

#include "vector"
#include "common/Module.h"

namespace love
{
namespace filesystem
{

class Filewatcher
{

public:

    enum EventType
    {
        EVENT_CREATED,
        EVENT_MODIFIED,
        EVENT_DELETED,
        EVENT_MOVED_FROM,
        EVENT_MOVED_TO,
        EVENT_ENUM_MAX
    };

	struct FileEvent
	{
		EventType Type;
		std::string Path;
	};

    virtual void watchFile(const std::string &path);
    void pump();
    void tryClose() const;

    Filewatcher();
    ~Filewatcher();

private:
	void dumpEvents();

#ifdef LOVE_LINUX
	std::map<int, std::string> watchers;
#endif

	std::vector<FileEvent> events;

    int fd = -1;
};

}
}