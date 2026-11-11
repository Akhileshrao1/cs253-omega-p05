# Structure A — Generic Segment Tree

**Project:** Aggregates a Fenwick Tree Cannot Support  
**Owner:** D1 — Structure A  
**Language:** C++  
**Status:** Design draft; awaiting Architect's INTERFACE.md

## 1. Objective

Implement a generic segment tree supporting associative aggregation operations with an identity element (a monoid).

The implementation must support:

- Range maximum with the number of occurrences of the maximum.
- Range GCD.
- Point assignment updates, including updates that decrease existing values.

## 2. Generic Design

The segment tree will be parameterized by an aggregation operation, its identity element, and the corresponding value type.

Each internal node stores the aggregate of its children's values.

The tree will support:

- Building from an initial array.
- Assigning a new value at a specified index.
- Querying an inclusive range.

Final C++ function signatures, indexing conventions, numeric types, and error handling will follow the Architect's INTERFACE.md.

## 3. Maximum with Count

Each node stores a pair:

(maximum value, number of occurrences)

For two pairs A = (a, ca) and B = (b, cb), the merge operation is:

- If a > b, return A.
- If b > a, return B.
- Otherwise, return (a, ca + cb).

Identity: (-infinity, 0).

For implementation, a representation of negative infinity compatible with the approved value type must be chosen.

## 4. Range GCD

Each node stores the GCD of the values in its interval.

Merge operation:

merge(a, b) = gcd(a, b)

Identity: 0.

The implementation will follow the team's agreed policy for negative values and zero.

## 5. Complexity Targets

| Operation | Complexity |
|---|---|
| Build | O(n) |
| Point assignment | O(log n) |
| Range maximum with count | O(log n) |
| Range GCD | O(log n) aggregation steps |
| Storage | O(n) |

GCD arithmetic has a separate bit-complexity cost depending on the values.

## 6. Structural Invariants

1. Every leaf represents one array element.
2. Every internal node stores the merge of its two children.
3. A point assignment updates the corresponding leaf and recomputes all affected ancestors.
4. A range query combines aggregates from disjoint intervals covering the requested range.
5. The result is correct because the merge operation is associative and has an identity.

## 7. Planned Testing

- Single-element arrays.
- All elements equal.
- Negative and zero values where supported.
- Point increases and decreases.
- Updates to first and last indices.
- Full-range and single-index queries.
- Randomized comparisons against a plain-array oracle.
- Large inputs with at least 1,000,000 elements and operations.

## 8. Pending Dependencies

Before implementation:

- Architect must complete INTERFACE.md.
- Confirm method signatures and indexing convention.
- Confirm numeric types and identity representation.
- Confirm how the application supplies maximum-count and GCD values.
- Confirm build and test commands.

## 9. D1 Derivation

A separate derivation will prove:

1. Maximum-with-count is associative and has identity (-infinity, 0).
2. A generic segment tree correctly answers range queries for any monoid.
3. Point assignment preserves the structural invariant.
4. The segment tree does not require inverse operations for range queries.

The final proof will be submitted as docs/derivations/D1.pdf.