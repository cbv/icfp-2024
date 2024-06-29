(declare-const b Bool) ; 1
(declare-const c Bool) ; 2
(declare-const d Bool) ; 4
(declare-const e Bool) ; 8
(declare-const f Bool) ; 16
(declare-const g Bool) ; 32
(declare-const h Bool) ; 64
(declare-const i Bool) ; 128
(declare-const j Bool) ; 256
(declare-const k Bool) ; 512
(declare-const l Bool) ; 1024
(declare-const m Bool) ; 2048
(declare-const n Bool) ; 4096
(declare-const o Bool) ; 8192
(declare-const p Bool) ; 16384
(declare-const q Bool) ; 32768
(declare-const r Bool) ; 65536
(declare-const s Bool) ; 131072
(declare-const t Bool) ; 262144
(declare-const u Bool) ; 524288
(declare-const v Bool) ; 1048576
(declare-const w Bool) ; 2097152
(declare-const x Bool) ; 4194304
(declare-const y Bool) ; 8388608
(declare-const z Bool) ; 16777216
(declare-const v26 Bool) ; 33554432
(declare-const v27 Bool) ; 67108864
(declare-const v28 Bool) ; 134217728
(declare-const v29 Bool) ; 268435456
(declare-const v30 Bool) ; 536870912
(declare-const v31 Bool) ; 1073741824
(declare-const v32 Bool) ; 2147483648
(declare-const v33 Bool) ; 4294967296
(declare-const v34 Bool) ; 8589934592
(declare-const v35 Bool) ; 17179869184
(declare-const v36 Bool) ; 34359738368
(declare-const v37 Bool) ; 68719476736
(declare-const v38 Bool) ; 137438953472
(declare-const v39 Bool) ; 274877906944
(declare-const v40 Bool) ; 549755813888

(declare-const value Int)

(assert
(= value
(+
(ite b 1 0)
(ite c 2 0)
(ite d 4 0)
(ite e 8 0)
(ite f 16 0)
(ite g 32 0)
(ite h 64 0)
(ite i 128 0)
(ite j 256 0)
(ite k 512 0)
(ite l 1024 0)
(ite m 2048 0)
(ite n 4096 0)
(ite o 8192 0)
(ite p 16384 0)
(ite q 32768 0)
(ite r 65536 0)
(ite s 131072 0)
(ite t 262144 0)
(ite u 524288 0)
(ite v 1048576 0)
(ite w 2097152 0)
(ite x 4194304 0)
(ite y 8388608 0)
(ite z 16777216 0)
(ite v26 33554432 0)
(ite v27 67108864 0)
(ite v28 134217728 0)
(ite v29 268435456 0)
(ite v30 536870912 0)
(ite v31 1073741824 0)
(ite v32 2147483648 0)
(ite v33 4294967296 0)
(ite v34 8589934592 0)
(ite v35 17179869184 0)
(ite v36 34359738368 0)
(ite v37 68719476736 0)
(ite v38 137438953472 0)
(ite v39 274877906944 0)
(ite v40 549755813888 0)
)))

(assert (and (or (not s) (not p)) (or (not o) (not j)) (or (not t) v37 m) (not v39) (or (not u) s) (or (not i) q (not y)) (or (not v29) (not v39)) (or (not c) t) p (or (not v37) t (not g)) (or z (not x)) (or (not r) v40 v) (or (not x) v35 y) (or (not v30) (not v28)) p (or (not v37) t g) (or (not d) (not l)) (or (not v35) (not d)) (or (not v29) v39 w) (or (not v27) (not k)) (or v28 (not i)) (or (not e) v39) (or k v26) (or w o p) (not n) (or v36 v28 (not v35)) (or (not i) q y) (or (not h) d (not v40)) (or w o p) (not n) (or (not i) q y) (or (not t) v37 m) (or k (not v26)) (or (not u) (not s)) (or (not i) (not q)) (or (not d) l (not x)) (or (not q) v37) (or (not v38) (not l)) (or (not v31) n o) (or (not v33) t) (or (not o) j (not v29)) (or g q) (or v36 v28 v35) (or (not t) (not v37)) (or (not f) g) (or k v26) (or (not d) l x) (or v26 v37 (not q)) (or w o (not p)) (or z x v38) (or v36 (not v28)) (or (not v35) d) (or (not v34) v35 (not v37)) (or (not f) (not g)) (or (not v33) (not t)) (or (not r) v40 v) (or (not v30) v28 e) (or (not v34) v35 v37) (or v28 i v29) (not v) (or (not s) p v39) v40 (or (not d) l x) (or (not r) (not v40)) (or (not v35) d) (or (not v33) t) (or z x (not v38)) (or (not v27) k) (or v36 v28 v35) (or (not h) d v40) (or (not l) v35) (or g q) (or (not v30) v28 e) (or (not l) (not v35)) (or (not v32) t) (or (not v37) (not t)) (or v26 (not v37)) (or (not v30) v28 (not e)) (not v) (or (not u) s) (or (not j) v31) (or v28 i v29) (or (not v31) n (not o)) (or (not y) v32 v35) (or m q) (or (not v29) v39 w) (or (not o) j v29) (not v39) (or (not e) (not v39)) (or (not v37) t g) (or (not x) (not v35)) (or (not v29) v39 (not w)) (or (not f) g) v40 (or z x v38) (or (not c) t) (or m q) (or (not x) v35 y) (or (not v31) (not n)) (or (not v34) (not v35)) (or (not v32) t) (or (not y) v32 v35) (or v26 v37 q) (or (not v38) l) (or (not q) v37) (or (not h) d v40) (or (not e) v39) (or (not h) (not d)) (or (not y) v32 (not v35)) (or (not v34) v35 v37) (or (not l) v35) b (or (not v27) k) (or (not x) v35 (not y)) (or (not s) p (not v39)) (or (not t) v37 (not m)) (or v26 v37 q) (or g (not q)) (or (not r) v40 (not v)) (or (not j) (not v31)) (or (not v32) (not t)) (or (not v38) l) (or (not j) v31) (or (not o) j v29) (or (not s) p v39) (or m (not q)) (or (not q) (not v37)) (or w (not o)) (or v28 i (not v29)) b (or (not y) (not v32)) (or (not c) (not t)) (or (not v31) n o)))

;; we need the MINIMUM value
(minimize value)

(check-sat)
(get-model)


