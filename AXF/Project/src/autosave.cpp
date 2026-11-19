#include "axf/project/autosave.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string_view>
#include <utility>

namespace axf::project
{
namespace
{
constexpr std::string_view kManifestMagic = "AXF-AUTOSAVE";

bool safeBaseName(std::string_view name) noexcept
{
    if (name.empty() || name.size() > 64)
    {
        return false;
    }
    return std::all_of(name.begin(), name.end(),
                       [](char value)
                       {
                           return (value >= 'a' && value <= 'z') ||
                                  (value >= 'A' && value <= 'Z') ||
                                  (value >= '0' && value <= '9') || value == '_' || value == '-';
                       });
}

std::filesystem::path revisionPath(const std::filesystem::path& directory,
                                   const std::string& baseName, std::size_t revision)
{
    return directory / (baseName + ".autosave." + std::to_string(revision));
}

std::filesystem::path manifestPath(const std::filesystem::path& directory,
                                   const std::string& baseName)
{
    return directory / (baseName + ".autosave.manifest");
}

core::Result<void> writeAtomic(const std::filesystem::path& target, std::string_view contents)
{
    std::error_code error;
    const auto temporary = target.string() + ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            return core::makeError(core::ErrorCode::IoError, "cannot create autosave temp file");
        }
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.flush();
        if (!output)
        {
            return core::makeError(core::ErrorCode::IoError, "cannot write autosave temp file");
        }
    }
    // POSIX rename() replaces atomically. Windows cannot rename over an
    // existing file, so fall back to remove+rename there.
    std::filesystem::rename(temporary, target, error);
    if (error)
    {
        error.clear();
        std::filesystem::remove(target, error);
        if (!error)
        {
            std::filesystem::rename(temporary, target, error);
        }
    }
    if (error)
    {
        std::filesystem::remove(temporary, error);
        return core::makeError(core::ErrorCode::IoError, "cannot commit autosave file");
    }
    return core::success();
}
} // namespace

ProjectAutosave::ProjectAutosave(std::size_t revisions) : revisions_(revisions == 0 ? 1 : revisions)
{
}

std::size_t ProjectAutosave::revisionCount() const noexcept
{
    return revisions_;
}

core::Result<void> ProjectAutosave::save(const ProjectDocument& document,
                                         const std::filesystem::path& directory,
                                         std::string baseName)
{
    if (!safeBaseName(baseName))
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "invalid autosave base name");
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot create autosave directory");
    }

    std::size_t latest = 1;
    const auto manifest = manifestPath(directory, baseName);
    {
        std::ifstream input(manifest, std::ios::binary);
        std::string magic;
        std::size_t previous = 0;
        std::size_t count = 0;
        if (input && input >> magic >> previous >> count && magic == kManifestMagic &&
            revisions_ > 0)
        {
            latest = previous % revisions_ + 1;
        }
    }

    const auto target = revisionPath(directory, baseName, latest);
    const auto written = writeAtomic(target, document.serialize());
    if (!written)
    {
        return written;
    }
    const std::string manifestContents = std::string(kManifestMagic) + " " +
                                         std::to_string(latest) + " " + std::to_string(revisions_) +
                                         "\n";
    const auto manifestWritten = writeAtomic(manifest, manifestContents);
    if (!manifestWritten)
    {
        return manifestWritten;
    }
    return core::success();
}

core::Result<ProjectDocument> ProjectAutosave::recover(const std::filesystem::path& directory,
                                                       const std::string& baseName) const
{
    if (!safeBaseName(baseName))
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "invalid autosave base name");
    }
    std::size_t latest = revisions_;
    {
        std::ifstream input(manifestPath(directory, baseName), std::ios::binary);
        std::string magic;
        std::size_t stored = 0;
        std::size_t count = 0;
        if (input && input >> magic >> stored >> count && magic == kManifestMagic && stored >= 1 &&
            stored <= revisions_)
        {
            latest = stored;
        }
    }

    for (std::size_t offset = 0; offset < revisions_; ++offset)
    {
        const std::size_t revision = (latest - 1 + revisions_ - offset) % revisions_ + 1;
        const auto path = revisionPath(directory, baseName, revision);
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            continue;
        }
        const std::string text((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
        auto document = ProjectDocument::parse(text);
        if (document)
        {
            return document;
        }
    }
    return core::makeError(core::ErrorCode::NotFound, "no valid autosave revision found");
}

core::Result<void> ProjectAutosave::clear(const std::filesystem::path& directory,
                                          const std::string& baseName) const
{
    if (!safeBaseName(baseName))
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "invalid autosave base name");
    }
    std::error_code error;
    std::error_code firstError;
    for (std::size_t revision = 1; revision <= revisions_; ++revision)
    {
        std::filesystem::remove(revisionPath(directory, baseName, revision), error);
        if (error && !firstError)
        {
            firstError = error;
        }
        error.clear();
    }
    std::filesystem::remove(manifestPath(directory, baseName), error);
    if (error && !firstError)
    {
        firstError = error;
    }
    if (firstError)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot remove autosave files");
    }
    return core::success();
}

} // namespace axf::project
