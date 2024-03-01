#ifndef _FORMATTER_H
#define _FORMATTER_H

#include <string>
#include <functional>

#include "str.h"
#include "regexp.h"

namespace formatter {
    std::string logFormat(const std::string& fmt, std::function<std::string(char)> chardef)
    {
	std::function<std::string(const std::string&)> formatField =
		[&](const std::string& inst) -> std::string
		{
                    static Regexp fieldRegexp("^%(-?\\d+)?([a-zA-Z%])$");

                    auto vec = fieldRegexp.exec(inst);
			
                    if (vec.size() != 3) {
                        throw FormatException(str::format("Failed to process: %s", inst.c_str()));
                    }

                    auto fieldWidth = vec[1];
                    char terminalCharacter = vec[2][0];

                    if (terminalCharacter == '%') {
                        return "%";
                    } else {

                        auto fmt = str::format("%%%ss", fieldWidth.c_str());
                        return str::format(fmt.c_str(), chardef(terminalCharacter).c_str());
                    }
		};

        return Regexp("%(-?\\d+)?[a-zA-Z%]").replaceAll(fmt, formatField);
    }
}

#endif
