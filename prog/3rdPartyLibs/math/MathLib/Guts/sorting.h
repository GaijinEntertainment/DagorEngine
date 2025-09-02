// © 2021 NVIDIA Corporation

#pragma once

// NOTE: returns p1 < p2 ? -1 : (p1 > p2 ? 1 : 0)

typedef int32_t (*pfn_cmp_qsort)(const void* p1, const void* p2);

// NOTE: true - swap, false - keep; a - left, b - right

template <class T>
inline bool Sort_default_less(const T& a, const T& b) {
    return a < b;
}

template <class T>
inline bool Sort_default_greater(const T& a, const T& b) {
    return a > b;
}

/*
bool Sort_multikey(const T& a, const T& b)
{
    if( a.property1 > b.property1 )
        return true;

    if( a.property1 == b.property1 )
    {
        if( a.property2 > b.property2 )
            return true;

        if( a.property2 == b.property2 )
            return a.property3 > b.property3;
    }

    return false;
}
*/

// NOTE: heap sort
// memory:      O(1)
// random:      +40% vs qsort
// sorted:      -30% vs qsort
// reversed:    -30% vs qsort

template <class T, bool (*cmp)(const T& a, const T& b)>
void Sort_heap(T* a, uint32_t n) {
    if (n < 2)
        return;

    uint32_t i = n >> 1;

    for (;;) {
        T t;

        if (i > 0)
            t = a[--i];
        else {
            if (--n == 0)
                return;

            t = a[n];
            a[n] = a[0];
        }

        uint32_t parent = i;
        uint32_t child = (i << 1) + 1;

        while (child < n) {
            if (child + 1 < n && cmp(a[child], a[child + 1]))
                child++;

            if (cmp(t, a[child])) {
                a[parent] = a[child];

                parent = child;
                child = (parent << 1) + 1;
            } else
                break;
        }

        a[parent] = t;
    }
}

// NOTE: merge sort
// memory:      O(n), t - temp array, return pointer to sorted array (can be a or t)
// random:      +130% vs qsort
// sorted:      +35% vs qsort
// reversed:    +40% vs qsort

template <class T, bool (*cmp)(const T& a, const T& b)>
T* Sort_merge(T* t, T* a, uint32_t n) {
    if (n < 2)
        return a;

    uint32_t n2 = n << 1;

    for (uint32_t size = 2; size < n2; size <<= 1) {
        T* tmp = t;

        for (uint32_t i = 0; i < n; i += size) {
            uint32_t j = i;
            uint32_t nj = i + (size >> 1);

            if (nj > n)
                nj = n;

            uint32_t k = nj;
            uint32_t nk = i + size;

            if (nk > n)
                nk = n;

            while (j < nj && k < nk)
                *tmp++ = cmp(a[j], a[k]) ? a[j++] : a[k++];

            nj -= j;
            nk -= k;

            if (nj) {
                memcpy(tmp, a + j, nj * sizeof(T));
                tmp += nj;
            }

            if (nk) {
                memcpy(tmp, a + k, nk * sizeof(T));
                tmp += nk;
            }
        }

        tmp = a;
        a = t;
        t = tmp;
    }

    return a;
}
