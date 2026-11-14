# D1 — Mathematical Derivation: Structure A (Segment Tree)



**Author:** Tharun A Surana


**Roll Number:** 250001074


**Role:** Structure A Owner


**Project:** Project 5 — Aggregates a Fenwick Tree Cannot Support




---



## 1. Introduction



Structure A uses a segment tree to answer range queries for associative operations with an identity element (monoids).



This derivation establishes two results:



1. The maximum-with-count operation forms a monoid with identity $(-\infty,0)$.

2. A segment tree correctly answers range queries for any monoid, without requiring an inverse operation or commutativity.



## 2. Maximum-with-Count Is a Monoid



### 2.1 Definition



Let each element be represented by a pair:



$$
(m,c)
$$



where:

- $m$ represents the maximum value.

- $c$ represents the number of occurrences of that maximum.



Define the binary operation $\oplus$ on two pairs as follows:



$$
(m_1,c_1)\oplus(m_2,c_2)
=
\begin{cases}
(m_1,c_1), & m_1>m_2,\\
(m_2,c_2), & m_2>m_1,\\
(m_1,c_1+c_2), & m_1=m_2.
\end{cases}
$$



In other words, the operation keeps the larger maximum. If both maxima are equal, it adds their counts.



### 2.2 Closure



Consider two valid summaries:



$$
A=(m_1,c_1),\qquad B=(m_2,c_2).
$$



There are three possible cases:



- If $m_1>m_2$, the result is $(m_1,c_1)$.

- If $m_2>m_1$, the result is $(m_2,c_2)$.

- If $m_1=m_2$, the result is $(m_1,c_1+c_2)$.



In every case, the result is a valid maximum-with-count summary.



Therefore, the operation satisfies closure.



### 2.3 Associativity



We need to prove:



$$
(A\oplus B)\oplus C
=
A\oplus(B\oplus C).
$$



Let the three summaries be:



$$
A=(m_1,c_1),\quad
B=(m_2,c_2),\quad
C=(m_3,c_3).
$$



Let:



$$
M=\max(m_1,m_2,m_3).
$$



The operation $\oplus$ always produces:



- The maximum value among the input summaries.

- The sum of the counts corresponding to that maximum value.



Therefore, both sides of the associativity equation produce:



$$
\left(
M,\;
\sum_{i:\,m_i=M}c_i
\right).
$$



Hence,



$$
(A\oplus B)\oplus C
=
A\oplus(B\oplus C).
$$



Therefore, the operation is associative.



**Intuition:** Regardless of how we group the summaries, we obtain the same overall maximum and the total number of occurrences of that maximum.



### 2.4 Identity Element



Consider the pair:



$$
E=(-\infty,0).
$$



We show that it acts as both a left and right identity.



For any valid summary $A=(m,c)$:



$$
A\oplus E
=
(m,c)\oplus(-\infty,0)
=
(m,c).
$$



Similarly,



$$
E\oplus A
=
(-\infty,0)\oplus(m,c)
=
(m,c).
$$



Thus,



$$
A\oplus E=E\oplus A=A.
$$



Therefore, $(-\infty,0)$ is the identity element.



Here, $-\infty$ represents a sentinel smaller than every valid input value, and it is reserved for the identity.



### 2.5 Conclusion



The maximum-with-count operation satisfies:



1. Closure.

2. Associativity.

3. Existence of an identity element $(-\infty,0)$.



Therefore, maximum-with-count forms a **monoid**.



---



## 3. Correctness of Segment Tree Range Queries for Any Monoid



### 3.1 Monoid Requirements



Let $S$ be a set with a binary operation $\oplus$ and identity element $e$.



For a monoid, the following properties hold:



**Associativity:**



$$
(a\oplus b)\oplus c
=
a\oplus(b\oplus c).
$$



**Identity:**



$$
a\oplus e=e\oplus a=a.
$$



Commutativity is not required. In general,



$$
a\oplus b\ne b\oplus a.
$$



A segment tree can support any operation satisfying these monoid properties.



### 3.2 Segment Tree Representation



Consider an array:



$$
a_1,a_2,\ldots,a_n.
$$



Each leaf of the segment tree represents one array element.



Every internal node represents a contiguous interval and stores the aggregate of all elements in that interval.



For a node representing interval $[l,r]$, its stored value is:



$$
T(l,r)=a_l\oplus a_{l+1}\oplus\cdots\oplus a_r.
$$



If the node has two children representing $[l,m]$ and $[m+1,r]$, then:



$$
T(l,r)=T(l,m)\oplus T(m+1,r).
$$



This maintains the original left-to-right order of the elements.



For unused leaves introduced by padding the tree, the stored value is the identity $e$.



### 3.3 Correctness of the Range Query



We want to answer a query over an interval $[L,R]$:



$$
Q(L,R)=a_L\oplus a_{L+1}\oplus\cdots\oplus a_R.
$$



A segment tree decomposes this query interval into a collection of disjoint tree-node intervals.



These intervals satisfy two properties:



1. Their union is exactly $[L,R]$.

2. They appear in left-to-right order without overlap.



Let these intervals be:



$$
I_1,I_2,\ldots,I_k.
$$



Let their stored values be:



$$
T_1,T_2,\ldots,T_k.
$$



Because the intervals are disjoint and cover the complete query range in order, we have:



$$
Q(L,R)=T_1\oplus T_2\oplus\cdots\oplus T_k.
$$



Each $T_i$ is already the correct aggregate of its corresponding interval.



By associativity, these aggregates can be combined using any valid parenthesization without changing the result.



Therefore, the segment tree returns exactly the aggregate of the requested range.



Hence, the range query is correct.



### 3.4 Why Commutativity Is Not Required



The query must preserve the original order of the array elements.



For example, suppose the operation is string concatenation:



$$
a\oplus b=ab.
$$



This operation is associative but not commutative because:



$$
ab\ne ba
$$



in general.



A segment tree still works correctly because it combines the selected intervals in left-to-right order.



Associativity allows us to change the grouping, but we do not change the order of the elements.



Thus, a segment tree supports non-commutative monoids as well.



### 3.5 Why an Inverse Operation Is Not Needed



Some data structures answer range queries using prefix aggregates.



For example, with addition:



$$
\text{RangeSum}(L,R)
=
\text{PrefixSum}(R)-\text{PrefixSum}(L-1).
$$



This works because addition has an inverse operation: subtraction.



However, a segment tree does not calculate a range result by subtracting two prefix results.



Instead, it directly identifies the disjoint tree intervals covering the requested range and combines their stored aggregates.



Therefore, only associativity and an identity element are required.



No inverse operation is needed.



This is important for maximum-with-count because there is no general inverse operation that can recover a range maximum by subtracting prefix maximum summaries.



### 3.6 Conclusion



A segment tree correctly answers range queries for any monoid because:



1. Each node stores the aggregate of its interval.

2. A query interval can be decomposed into disjoint tree-node intervals.

3. These intervals are combined in their original left-to-right order.

4. Associativity guarantees that the grouping does not affect the result.

5. The identity element handles unused or empty portions.

6. No inverse operation or commutativity is required.



Therefore, a segment tree is a generic data structure for range queries over arbitrary monoids.



---



## 4. Overall Conclusion



The maximum-with-count operation is a monoid with identity $(-\infty,0)$.



Furthermore, segment trees can correctly answer range queries for any monoid, including non-commutative monoids, without requiring an inverse operation.



These results establish the mathematical foundation of Structure A and justify using the same generic segment tree implementation for maximum-with-count and other associative operations.
