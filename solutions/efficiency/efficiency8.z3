
(declare-const b Bool)  ; 1
(declare-const c Bool)  ; 2
(declare-const d Bool)  ; 4
(declare-const e Bool)  ; 8
(declare-const f Bool)  ; 16
(declare-const g Bool)  ; 32
(declare-const h Bool)  ; 64
(declare-const i Bool)  ; 128
(declare-const j Bool)  ; 256
(declare-const k Bool)  ; 512
(declare-const l Bool)  ; 1024
(declare-const m Bool)  ; 2048
(declare-const n Bool)  ; 4096
(declare-const o Bool)  ; 8192
(declare-const p Bool)  ; 16384
(declare-const q Bool)  ; 32768
(declare-const r Bool)  ; 65536
(declare-const s Bool)  ; 131072
(declare-const t Bool)  ; 262144
(declare-const u Bool)  ; 524288
(declare-const v Bool)  ; 1048576
(declare-const w Bool)  ; 2097152
(declare-const x Bool)  ; 4194304
(declare-const y Bool)  ; 8388608
(declare-const z Bool)  ; 16777216
(declare-const v26 Bool)  ; 33554432
(declare-const v27 Bool)  ; 67108864
(declare-const v28 Bool)  ; 134217728
(declare-const v29 Bool)  ; 268435456
(declare-const v30 Bool)  ; 536870912
(declare-const v31 Bool)  ; 1073741824
(declare-const v32 Bool)  ; 2147483648
(declare-const v33 Bool)  ; 4294967296
(declare-const v34 Bool)  ; 8589934592
(declare-const v35 Bool)  ; 17179869184
(declare-const v36 Bool)  ; 34359738368
(declare-const v37 Bool)  ; 68719476736
(declare-const v38 Bool)  ; 137438953472
(declare-const v39 Bool)  ; 274877906944
(declare-const v40 Bool)  ; 549755813888
(declare-const v41 Bool)  ; 1099511627776
(declare-const v42 Bool)  ; 2199023255552
(declare-const v43 Bool)  ; 4398046511104
(declare-const v44 Bool)  ; 8796093022208
(declare-const v45 Bool)  ; 17592186044416
(declare-const v46 Bool)  ; 35184372088832
(declare-const v47 Bool)  ; 70368744177664
(declare-const v48 Bool)  ; 140737488355328
(declare-const v49 Bool)  ; 281474976710656
(declare-const v50 Bool)  ; 562949953421312

;; the binary number (as a sum of the powers of two)
(declare-const value Int)
(assert (= value
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
(ite v41 1099511627776 0)
(ite v42 2199023255552 0)
(ite v43 4398046511104 0)
(ite v44 8796093022208 0)
(ite v45 17592186044416 0)
(ite v46 35184372088832 0)
(ite v47 70368744177664 0)
(ite v48 140737488355328 0)
(ite v49 281474976710656 0)
(ite v50 562949953421312 0)
)))


(assert (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (and (or (or (not d) v36) h) (or (or (not d) (not v42)) (not v48))) (or (or (not v49) (not v47)) (not v41))) (or (or i (not v40)) r)) (or (or (not v) (not v31)) (not v39))) (or (or v36 (not w)) v49)) (or (or v27 v38) o)) (or (or p (not s)) g)) (or (or g h) (not v43))) (or (or v34 (not h)) x)) (or (or c o) (not n))) (or (or c v47) (not v42))) (or (or (not v33) (not v35)) d)) (or (or v44 v40) v49)) (or (or v50 v36) v31)) (or (or (not v36) (not d)) (not v37))) (or (or v26 (not v29)) v43)) (or (or p v29) (not v45))) (or (or y (not l)) s)) (or (or (not v47) (not v26)) g)) (or (or (not v50) (not v33)) (not k))) (or (or v32 g) q)) (or (or (not v34) v37) v41)) (or (or h (not v28)) (not r))) (or (or (not v44) v46) t)) (or (or h w) (not v48))) (or (or d v39) v34)) (or (or v31 v46) (not v43))) (or (or (not v27) v32) x)) (or (or v37 (not v50)) (not s))) (or (or u f) l)) (or (or (not v45) (not y)) g)) (or (or (not v34) (not x)) (not o))) (or (or (not w) v) u)) (or (or (not r) v50) y)) (or (or (not z) (not y)) (not v27))) (or (or d v35) v)) (or (or (not v26) v47) (not v36))) (or (or (not v28) (not v45)) v49)) (or (or (not v) (not g)) m)) (or (or (not r) (not p)) (not v39))) (or (or v41 c) (not o))) (or (or z v36) (not x))) (or (or (not v39) (not d)) (not v40))) (or (or v50 u) v35)) (or (or v27 v31) (not v39))) (or (or v45 (not p)) (not v40))) (or (or v34 v50) v35)) (or (or (not b) (not v48)) m)) (or (or s (not v35)) (not v30))) (or (or v27 (not y)) (not z))) (or (or (not e) (not v33)) (not m))) (or (or (not v43) (not y)) (not v37))) (or (or (not v37) v31) (not v44))) (or (or (not j) (not v38)) o)) (or (or v33 (not q)) v34)) (or (or e (not v35)) (not f))) (or (or (not d) (not v)) (not t))) (or (or (not v35) (not v36)) (not v29))) (or (or h (not v43)) v36)) (or (or v30 o) v41)) (or (or (not v35) (not y)) (not h))) (or (or v35 (not v42)) g)) (or (or (not b) (not p)) v39)) (or (or v27 v49) (not q))) (or (or (not v37) v49) (not k))) (or (or v50 (not v46)) (not d))) (or (or (not v41) u) v34)) (or (or (not b) x) v28)) (or (or (not m) (not v30)) (not u))) (or (or (not y) v29) (not v37))) (or (or m f) (not v44))) (or (or (not g) (not c)) v48)) (or (or (not c) (not v49)) (not v43))) (or (or b (not v50)) y)) (or (or (not h) (not v50)) (not v44))) (or (or (not v41) v43) e)) (or (or n p) (not l))) (or (or (not d) (not l)) x)) (or (or v33 v48) v41)) (or (or j x) (not v49))) (or (or (not v43) v47) b)) (or (or (not v40) q) (not v29))) (or (or v30 t) d)) (or (or t (not v34)) v48)) (or (or (not q) (not v44)) o)) (or (or v38 (not v45)) (not m))) (or (or (not e) (not o)) (not v31))) (or (or (not v48) v35) (not b))) (or (or v45 (not n)) t)) (or (or j v42) (not h))) (or (or (not b) (not p)) i)) (or (or (not n) (not v44)) (not o))) (or (or (not v43) (not v37)) (not v31))) (or (or (not v27) (not v29)) v47)) (or (or h e) r)) (or (or h k) v35)) (or (or (not z) u) r)) (or (or v35 (not f)) (not v42))) (or (or (not v50) y) (not f))) (or (or (not v) (not v26)) c)) (or (or (not i) v45) (not v))) (or (or (not q) v33) v49)) (or (or (not v38) g) q)) (or (or f v) v37)) (or (or i v38) v31)) (or (or (not v) v33) o)) (or (or u v40) (not f))) (or (or (not v29) (not j)) v31)) (or (or (not h) v42) (not w))) (or (or (not v48) i) v26)) (or (or v48 (not v38)) v33)) (or (or (not v34) v49) v46)) (or (or (not o) (not v46)) z)) (or (or (not v46) e) s)) (or (or v36 (not m)) (not v31))) (or (or m (not s)) o)) (or (or (not h) v46) (not q))) (or (or j (not i)) h)) (or (or v49 (not v42)) (not w))) (or (or w (not p)) v38)) (or (or v34 (not v41)) v47)) (or (or w (not v26)) v32)) (or (or (not z) (not v45)) (not v))) (or (or (not v26) v32) (not l))) (or (or p v26) (not z))) (or (or (not b) v46) z)) (or (or (not o) (not v31)) v30)) (or (or (not j) (not w)) m)) (or (or (not s) v26) (not v35))) (or (or (not q) (not v32)) (not v))) (or (or v31 (not v49)) (not v))) (or (or l j) v41)) (or (or (not n) (not v30)) t)) (or (or (not k) e) g)) (or (or (not e) d) (not w))) (or (or (not z) (not v50)) (not s))) (or (or (not v40) e) j)) (or (or v37 u) v46)) (or (or (not v27) w) (not v29))) (or (or v34 o) d)) (or (or d (not v31)) u)) (or (or (not v50) c) (not v26))) (or (or r (not v29)) v38)) (or (or (not v49) m) (not v41))) (or (or p (not v35)) (not v43))) (or (or (not w) (not x)) (not v49))) (or (or (not j) v33) v48)) (or (or v26 v29) v35)) (or (or v27 (not v50)) v37)) (or (or (not h) v46) (not v43))) (or (or (not v46) (not v37)) (not i))) (or (or (not v40) v36) (not y))) (or (or (not v44) v46) p)) (or (or (not d) v36) (not q))) (or (or (not v48) j) v43)) (or (or (not z) (not e)) v44)) (or (or (not w) v37) (not h))) (or (or (not v31) (not r)) (not w))) (or (or (not l) (not v48)) r)) (or (or x v34) (not v28))) (or (or x (not v48)) (not v39))) (or (or (not v37) (not b)) (not x))) (or (or (not t) v27) o)) (or (or (not w) v33) (not g))) (or (or (not g) (not v32)) (not v26))) (or (or s (not u)) (not v46))) (or (or v43 w) v27)) (or (or (not n) v34) v49)) (or (or (not v35) (not v46)) d)) (or (or v32 v39) (not v43))) (or (or g (not v39)) (not j))) (or (or v27 v39) (not q))) (or (or z (not r)) (not p))) (or (or (not v43) v27) v34)) (or (or (not g) v49) f)) (or (or (not v38) l) o)) (or (or v40 (not v38)) v47)) (or (or v37 (not o)) r)) (or (or v39 v29) v36)) (or (or (not v39) (not v28)) b)) (or (or (not s) o) (not q))) (or (or (not v40) v50) p)) (or (or v37 (not v42)) s)) (or (or (not n) v31) v33)) (or (or c (not v42)) v33)) (or (or i (not d)) (not w))) (or (or b x) (not v31))) (or (or (not u) (not v45)) v26)) (or (or v42 l) v49)) (or (or v29 l) (not v43))) (or (or (not u) (not v)) v30)) (or (or x v45) (not v35))) (or (or v38 (not v30)) (not o))) (or (or (not j) v48) (not v29))) (or (or l (not s)) (not x))) (or (or (not v41) (not b)) (not v29))) (or (or f v41) v26)) (or (or v44 (not v30)) (not h))) (or (or v38 (not g)) (not v41))) (or (or v46 v48) (not p))) (or (or (not s) (not k)) (not v47))) (or (or v38 v46) (not v32))) (or (or (not v32) v46) m)) (or (or v31 v40) o)) (or (or (not s) c) v49)) (or (or v28 (not v38)) v27)) (or (or (not q) (not v)) o)) (or (or (not v29) p) m)) (or (or v49 v34) f)) (or (or o w) (not m))) (or (or v30 v33) u)) (or (or (not y) w) z)) (or (or e (not v48)) (not x))) (or (or (not v30) (not v36)) j)) (or (or v44 m) (not v35))) (or (or v38 d) (not v))) (or (or (not l) v33) v49)))

;; we need the MINIMUM value (the solution is not unique)
(minimize value)

(check-sat)
(get-model)

