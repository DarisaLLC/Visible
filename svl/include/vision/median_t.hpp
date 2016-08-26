

  template<class T>
      inline void Median3x3_t (const T *l0, const T *l1, const T *l2, T *med)
 {
        T a0 = *l0++;
        T a1 = *l0++;
        T a2 = *l0;
        T b0 = *l1++;
        T b1 = *l1++;
        T b2 = *l1;
        T c0 = *l2++;
        T c1 = *l2++;
        T c2 = *l2;

        T A1 = std::min(a1, a2);
        a2 = std::max(a1, a2);
        a1 = std::max(a0, A1);
        a0 = std::min(a0, A1);
        A1 = std::min(a1, a2);
        a2 = std::max(a1, a2);

        T B1 = std::min(b1, b2);
        b2 = std::max(b1, b2);
        b1 = std::max(b0, B1);
        b0 = std::min(b0, B1);
        B1 = std::min(b1, b2);
        b2 = std::max(b1, b2);

        T C1 = std::min(c1, c2);
        c2 = std::max(c1, c2);
        c1 = std::max(c0, C1);
        c0 = std::min(c0, C1);
        C1 = std::min(c1, c2);
        c2 = std::max(c1, c2);

        a0 = std::max(a0, std::max(b0, c0));
        a2 = std::min(a2, std::max(b2, c2));
        b1 = std::min(B1, C1);
        b2 = std::max(B1, C1);
        b1 = std::max(A1, b1);
        a1 = std::min(b1, b2);

        b1 = std::min(a1, a2);
        b2 = std::max(a1, a2);
        b1 = std::max(a0, b1);
        *med = std::min(b1, b2);
 }



