#include "axf/media/audio_file.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace axf::media
{

const char* formatName(AudioFileFormat format) noexcept
{
    switch (format)
    {
    case AudioFileFormat::Unknown:
        return "unknown";
    case AudioFileFormat::Wav:
        return "wav";
    case AudioFileFormat::Aiff:
        return "aiff";
    case AudioFileFormat::Flac:
        return "flac";
    }
    return "unknown";
}

AudioFileFormat formatFromExtension(const std::string& path) noexcept
{
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= path.size())
    {
        return AudioFileFormat::Unknown;
    }
    std::string extension = path.substr(dot + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension == "wav")
        return AudioFileFormat::Wav;
    if (extension == "aif" || extension == "aiff")
        return AudioFileFormat::Aiff;
    if (extension == "flac")
        return AudioFileFormat::Flac;
    return AudioFileFormat::Unknown;
}

} // namespace axf::media
