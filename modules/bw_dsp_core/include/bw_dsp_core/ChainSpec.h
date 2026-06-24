// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file ChainSpec.h
 * @brief Constexpr signal chain topology declarations
 *
 * Compile-time verified DSP chain topology. Plugins declare their signal
 * chain as constexpr data; static_assert validates node counts match
 * processBlockImpl(). Zero runtime cost.
 *
 * Usage:
 *   static constexpr auto kSignalChain = bws::dsp::Serial{
 *       bws::dsp::Node<MyFilter>{"filter"},
 *       bws::dsp::Conditional<MyComp>{"comp", "compEnabled"},
 *       bws::dsp::Inline{"trim", "Output Trim"}
 *   };
 *   static_assert(kSignalChain.nodeCount() == 3);
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace bws::dsp
{

// ─── Type Trait: is_specialization ──────────────────────────────────────
// Standard pattern for detecting template specializations at compile time.

template <typename T, template <typename...> class Template>
struct is_specialization : std::false_type
{};

template <template <typename...> class Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type
{};

template <typename T, template <typename...> class Template>
inline constexpr bool is_specialization_v = is_specialization<T, Template>::value;

// ─── Forward Declarations ──────────────────────────────────────────────

template <typename Module>
struct Node;
template <typename Module>
struct Conditional;
struct Inline;
struct Merge;

// ─── Concept: IsLinearNode ─────────────────────────────────────────────
// Constrains Serial to only accept Node, Conditional, Inline.

template <typename T>
concept IsLinearNode = is_specialization_v<T, Node> || is_specialization_v<T, Conditional> || std::same_as<T, Inline>;

// ─── Detail: Counting Helpers (forward-declared) ───────────────────────

namespace detail
{
template <typename T>
constexpr size_t countNodes();
template <typename T>
constexpr size_t countElements();
} // namespace detail

// ─── Node Types ────────────────────────────────────────────────────────

template <typename Module>
struct Node
{
    const char* id;
};

template <typename Module>
struct Conditional
{
    const char* id;
    const char* parameter; // APVTS parameter ID that gates this node
};

struct Inline
{
    const char* id;
    const char* label;
    const char* description = nullptr; // optional detail for panel
};

// ─── Topology: Serial ──────────────────────────────────────────────────

template <IsLinearNode... Nodes>
struct Serial
{
    std::tuple<Nodes...> nodes;

    constexpr Serial(Nodes... ns)
        : nodes {std::move(ns)...}
    {}

    static constexpr size_t nodeCount() { return (detail::countNodes<Nodes>() + ... + size_t {0}); }

    static constexpr size_t elementCount() { return (detail::countElements<Nodes>() + ... + size_t {0}); }
};

// CTAD: deduction handled by constructor (Serial(Nodes...) -> Serial<Nodes...>)

// ─── Topology: Split / Branch / Merge ──────────────────────────────────

template <typename... Branches>
struct Split
{
    const char* id;

    static constexpr size_t nodeCount() { return (detail::countNodes<Branches>() + ... + size_t {0}); }

    static constexpr size_t elementCount() { return 1 + (detail::countElements<Branches>() + ... + size_t {0}); }
};

template <typename... Nodes>
struct Branch
{
    const char* label;

    static constexpr size_t nodeCount() { return (detail::countNodes<Nodes>() + ... + size_t {0}); }

    static constexpr size_t elementCount() { return (detail::countElements<Nodes>() + ... + size_t {0}); }
};

enum class MergeStrategy
{
    Crossfade,
    Sum,
    Replace
};

struct Merge
{
    const char* splitId;
    MergeStrategy strategy;
};

// ─── Topology: Feedback ────────────────────────────────────────────────

template <typename... Nodes>
struct Feedback
{
    const char* id;

    static constexpr size_t nodeCount() { return (detail::countNodes<Nodes>() + ... + size_t {0}); }

    static constexpr size_t elementCount() { return (detail::countElements<Nodes>() + ... + size_t {0}); }
};

// ─── Topology: Custom ──────────────────────────────────────────────────

template <typename... Elements>
struct Custom
{
    std::tuple<Elements...> elements;

    constexpr Custom(Elements... es)
        : elements {std::move(es)...}
    {}

    static constexpr size_t nodeCount() { return (detail::countNodes<Elements>() + ... + size_t {0}); }

    static constexpr size_t elementCount() { return (detail::countElements<Elements>() + ... + size_t {0}); }
};

// CTAD: deduction handled by constructor (Custom(Elements...) -> Custom<Elements...>)

// ─── Detail: Counting Helpers (implementation) ─────────────────────────

namespace detail
{

template <typename T>
constexpr size_t countNodes()
{
    if constexpr (is_specialization_v<T, Node> || is_specialization_v<T, Conditional> || std::same_as<T, Inline>)
    {
        return 1;
    }
    else if constexpr (std::same_as<T, Merge>)
    {
        return 0;
    }
    else
    {
        // Compound types (Serial, Split, Branch, Feedback, Custom)
        // all provide static nodeCount().
        return T::nodeCount();
    }
}

template <typename T>
constexpr size_t countElements()
{
    if constexpr (is_specialization_v<T, Node> || is_specialization_v<T, Conditional> || std::same_as<T, Inline>)
    {
        return 1;
    }
    else if constexpr (std::same_as<T, Merge>)
    {
        return 1;
    }
    else
    {
        return T::elementCount();
    }
}

} // namespace detail

} // namespace bws::dsp
