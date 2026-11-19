#pragma once

#include "axf/core/error.hpp"

#include <optional>
#include <utility>

namespace axf::core
{

/// Simple expected/result type for APIs that can fail. Prefer this over
/// exceptions at module boundaries. Do not use with allocating T in the
/// audio callback; use status codes directly there.
template <class T> class [[nodiscard]] Result final
{
  public:
    using value_type = T;

    Result(T value) : value_(std::move(value))
    {
    }
    Result(Error error) : error_(error)
    {
    }

    [[nodiscard]] bool ok() const noexcept
    {
        return value_.has_value();
    }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return ok();
    }

    [[nodiscard]] T& value() &
    {
        return *value_;
    }
    [[nodiscard]] const T& value() const&
    {
        return *value_;
    }
    [[nodiscard]] T&& value() &&
    {
        return std::move(*value_);
    }

    [[nodiscard]] Error& error() & noexcept
    {
        return error_;
    }
    [[nodiscard]] const Error& error() const& noexcept
    {
        return error_;
    }

    [[nodiscard]] T valueOr(T fallback) const
    {
        return ok() ? *value_ : std::move(fallback);
    }

  private:
    std::optional<T> value_;
    Error error_{};
};

template <> class [[nodiscard]] Result<void> final
{
  public:
    Result() = default;
    Result(Error error) : error_(error)
    {
    }

    [[nodiscard]] bool ok() const noexcept
    {
        return !static_cast<bool>(error_);
    }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return ok();
    }

    [[nodiscard]] Error& error() & noexcept
    {
        return error_;
    }
    [[nodiscard]] const Error& error() const& noexcept
    {
        return error_;
    }

  private:
    Error error_{};
};

[[nodiscard]] inline Result<void> success() noexcept
{
    return Result<void>{};
}

} // namespace axf::core
