#!/usr/bin/env python3
# =============================================================================
# 修正紀錄 (2026-10-06)：以下三處為修改點，程式其餘邏輯未動。
#   [修正1] 新增 q_of()，統一 sqrt 的分支選擇，所有原本的
#           np.sqrt(kp**2 - eps*k0**2 + 0j)/k0 都改為呼叫 q_of()。
#           原因：原寫法在 eps 有虛部(吸收層)時 Im(q) 會變號，
#                 導致各層 e^{+ikz} / e^{-ikz} 慣例不一致，反射係數算錯。
#           副作用：無損時 q 與舊版互為複共軛，|r| 不變、相位符號相反。
#   [修正2] main() 的入射角原本 theta=30.0 直接丟進 np.sin (弧度)，
#           實際等效角約 81 度。改為度數輸入後用 np.deg2rad 轉換。
#   [修正3] main() 原本 RF/RB 硬寫 "TM"、Hybrid 硬寫 "TE"，
#           比較的是不同偏極化。全部改用 polarization 變數。
# =============================================================================
import numpy as np
import matplotlib.pyplot as plt


# [修正1] 新增函式：統一的 q 計算，固定 sqrt 分支 ----------------------------
def q_of(eps, k0, kp2):
    """
    Normalized decay constant q = -i*kz/k0, with kz = sqrt(eps*k0**2 - kp**2).
    The branch is fixed by Im(kz) >= 0, so that
      - propagating wave : q = -i*n*cos(theta)   (pure imaginary)
      - evanescent wave  : q = +sqrt(kp**2 - eps*k0**2)/k0 > 0  (decays)
      - absorbing layer  : Re(q) > 0 and Im(q) continuous with the lossless limit
    Using np.sqrt(kp2 - eps*k0**2 + 0j) directly flips the sign of Im(q) as soon
    as eps acquires an imaginary part, which mixes e^{+ikz} and e^{-ikz}
    conventions between layers.
    """
    kz = np.sqrt(eps * k0**2 - kp2 + 0j)   # 傳播波: 正實數; evanescent: 正虛數
    if np.imag(kz) < 0:
        kz = -kz                            # 強制 Im(kz) >= 0 (往下衰減)
    return -1j * kz / k0                    # q = -i kz/k0, Re(q) >= 0
# -----------------------------------------------------------------------------


def r_te(qi, qj):
    """TE Fresnel reflection (using q = -i*kz/k0, see q_of)."""
    return (qi - qj) / (qi + qj)

def r_tm(qi, qj, ei, ej):
    """TM Fresnel reflection (using q = -i*kz/k0, see q_of)."""
    return (qi/ei - qj/ej) / (qi/ei + qj/ej)

def RF_multilayer(n_list, d_list, polarization, k0, kp, target_layer):
    """
    RF: reflection looking downward starting from target_layer.
    n_list: [n1, n2, ..., nN] (N layers, last is substrate)
    d_list: [d1, d2, ..., d_{N-1}] (thicknesses, last has no thickness)
    target_layer: index where you站著往下看 (0-based)
    """
    e_list = [n**2 for n in n_list]
    kp2 = kp**2
    r_eff = 0.0 + 0.0j

    # 從最底層往上到 target_layer
    for L in range(len(n_list)-1, target_layer, -1):
        eps_up   = e_list[L-1]
        eps_down = e_list[L]

        q_up   = q_of(eps_up,   k0, kp2)   # [修正1] 原: np.sqrt(kp2 - eps_up   * k0**2 + 0j) / k0
        q_down = q_of(eps_down, k0, kp2)   # [修正1] 原: np.sqrt(kp2 - eps_down * k0**2 + 0j) / k0
        xL     = np.exp(-q_up * k0 * d_list[L-1])

        if polarization.upper() == "TE":
            wn = q_down / q_up
        else:  # TM
            wn = (q_down / q_up) * (eps_up / eps_down)

        wm = 1.0
        wp = wm + wn
        wq = wm - wn

        al = (wp + r_eff * wq) / 2.0
        bl = (wq + r_eff * wp) / 2.0
        r_eff = xL * (bl / al) * xL

    return r_eff


def RF_multilayer_hybrid_finite(n_list, d_list, polarization, k0, kp, target_layer, finite_layer, n_bounce):
    """
    Hybrid RF recursion:
    - same as RF_multilayer for all layers
    - but when current recursion index L is in finite_layer, use finite-bounce update

    finite_layer follows the recursion index L in RF_multilayer loop:
    for L in range(len(n_list)-1, target_layer, -1).

    Parameters
    ----------
    finite_layer : int or sequence of int
        Recursion layer(s) where finite-bounce approximation is used.
    n_bounce : int or sequence of int
        Number of finite bounces for each selected layer.
        If finite_layer is a sequence, n_bounce must be either:
        - one int (same n_bounce for all finite_layer), or
        - a sequence with the same length as finite_layer.
    """
    finite_layers = np.atleast_1d(finite_layer).astype(int).tolist()
    n_bounces = np.atleast_1d(n_bounce).astype(int).tolist()

    if len(finite_layers) == 0:
        raise ValueError("finite_layer cannot be empty")

    if len(n_bounces) == 1 and len(finite_layers) > 1:
        n_bounces = n_bounces * len(finite_layers)
    elif len(n_bounces) != len(finite_layers):
        raise ValueError("n_bounce must be one value or match finite_layer length")

    finite_map = dict(zip(finite_layers, n_bounces))
    if any(nb < 1 for nb in finite_map.values()):
        raise ValueError("all n_bounce values must be >= 1")

    e_list = [n**2 for n in n_list]
    kp2 = kp**2
    r_eff = 0.0 + 0.0j

    for L in range(len(n_list)-1, target_layer, -1):
        eps_up = e_list[L-1]
        eps_down = e_list[L]

        q_up = q_of(eps_up, k0, kp2)       # [修正1] 原: np.sqrt(kp2 - eps_up * k0**2 + 0j) / k0
        q_down = q_of(eps_down, k0, kp2)   # [修正1] 原: np.sqrt(kp2 - eps_down * k0**2 + 0j) / k0
        xL = np.exp(-q_up * k0 * d_list[L-1])

        if polarization.upper() == "TE":
            wn = q_down / q_up
        else:  # TM
            wn = (q_down / q_up) * (eps_up / eps_down)

        wm = 1.0
        wp = wm + wn
        wq = wm - wn

        if L in finite_map:
            n_bounce_L = finite_map[L]

            # Interface reflection from layer (L-1) -> L
            r01 = wq / wp
            r10 = -r01

            # Finite-bounce series at the current interface side
            # (same normalization as bl/al in the closed-form branch)
            ratio = r10 * r_eff
            first_internal = (1.0 - r01**2) * r_eff

            total = r01
            term = first_internal
            for _ in range(n_bounce_L):
                total += term
                term *= ratio
            r_eff = xL * total * xL
        else:
            al = (wp + r_eff * wq) / 2.0
            bl = (wq + r_eff * wp) / 2.0
            r_eff = xL * (bl / al) * xL

    return r_eff


def RB_multilayer(n_list, d_list, polarization, k0, kp, target_layer):
    """
    RB: reflection looking upward starting from target_layer.
    n_list: [n1, n2, ..., nN] (N layers, last is substrate)
    d_list: [d1, d2, ..., d_{N-1}] (thicknesses, last has no thickness)
    target_layer: index where you站著往上看 (0-based)
    """
    e_list = [n**2 for n in n_list]
    kp2 = kp**2
    r_eff = 0.0 + 0.0j

    # 從最上層往下到 target_layer
    for L in range(0, target_layer):
        eps_up   = e_list[L]
        eps_down = e_list[L+1]

        q_up   = q_of(eps_up,   k0, kp2)   # [修正1] 原: np.sqrt(kp2 - eps_up   * k0**2 + 0j) / k0
        q_down = q_of(eps_down, k0, kp2)   # [修正1] 原: np.sqrt(kp2 - eps_down * k0**2 + 0j) / k0
        xL     = np.exp(-q_up * k0 * d_list[L])

        if polarization.upper() == "TE":
            wn = q_up / q_down
        else:  # TM
            wn = (q_up / q_down) * (eps_down / eps_up)

        wm = 1.0
        wp = wm + wn
        wq = wm - wn

        bl = (wq + wp * xL * r_eff * xL)
        al = (wp + wq * xL * r_eff * xL)
        r_eff = bl / al

    return r_eff


def RB_multilayer_hybrid_finite(n_list, d_list, polarization, k0, kp, target_layer, finite_layer, n_bounce):
    """
    Hybrid RB recursion:
    - same as RB_multilayer for all layers
    - but when current recursion index L is in finite_layer, use finite-bounce update

    finite_layer follows the recursion index L in RB_multilayer loop:
    for L in range(0, target_layer).

    Parameters
    ----------
    finite_layer : int or sequence of int
        Recursion layer(s) where finite-bounce approximation is used.
    n_bounce : int or sequence of int
        Number of finite bounces for each selected layer.
        If finite_layer is a sequence, n_bounce must be either:
        - one int (same n_bounce for all finite_layer), or
        - a sequence with the same length as finite_layer.
    """
    finite_layers = np.atleast_1d(finite_layer).astype(int).tolist()
    n_bounces = np.atleast_1d(n_bounce).astype(int).tolist()

    if len(finite_layers) == 0:
        raise ValueError("finite_layer cannot be empty")

    if len(n_bounces) == 1 and len(finite_layers) > 1:
        n_bounces = n_bounces * len(finite_layers)
    elif len(n_bounces) != len(finite_layers):
        raise ValueError("n_bounce must be one value or match finite_layer length")

    finite_map = dict(zip(finite_layers, n_bounces))
    if any(nb < 1 for nb in finite_map.values()):
        raise ValueError("all n_bounce values must be >= 1")

    e_list = [n**2 for n in n_list]
    kp2 = kp**2
    r_eff = 0.0 + 0.0j

    for L in range(0, target_layer):
        eps_up = e_list[L]
        eps_down = e_list[L + 1]

        q_up = q_of(eps_up, k0, kp2)       # [修正1] 原: np.sqrt(kp2 - eps_up * k0**2 + 0j) / k0
        q_down = q_of(eps_down, k0, kp2)   # [修正1] 原: np.sqrt(kp2 - eps_down * k0**2 + 0j) / k0
        xL = np.exp(-q_up * k0 * d_list[L])

        if polarization.upper() == "TE":
            wn = q_up / q_down
        else:  # TM
            wn = (q_up / q_down) * (eps_down / eps_up)

        wm = 1.0
        wp = wm + wn
        wq = wm - wn

        if L in finite_map:
            n_bounce_L = finite_map[L]

            y = xL * r_eff * xL
            r01 = wq / wp
            r10 = -r01

            ratio = r10 * y
            first_internal = (1.0 - r01**2) * y

            total = r01
            term = first_internal
            for _ in range(n_bounce_L):
                total += term
                term *= ratio
            r_eff = total
        else:
            bl = (wq + wp * xL * r_eff * xL)
            al = (wp + wq * xL * r_eff * xL)
            r_eff = bl / al

    return r_eff


def r_closed_form_single_film(n_list, d_list, polarization, k0, kp):
    """
    Closed-form reflection for one spacer layer (layer 1) above an arbitrary
    multilayer reflector below it:
    r0 = (r01 + rF) / (1 + r01*rF)
    where rF is obtained from RF_multilayer(..., target_layer=1).
    where q = q_of(eps, k0, kp**2) = -i*kz/k0.
    """
    if len(n_list) < 3 or len(d_list) != len(n_list) - 1:
        raise ValueError("require at least 3 layers and d_list length = len(n_list)-1")

    e0, e1 = n_list[0]**2, n_list[1]**2
    q0 = q_of(e0, k0, kp**2)   # [修正1] 原: np.sqrt(kp**2 - e0 * k0**2 + 0j) / k0
    q1 = q_of(e1, k0, kp**2)   # [修正1] 原: np.sqrt(kp**2 - e1 * k0**2 + 0j) / k0

    if polarization.upper() == "TE":
        r01 = r_te(q0, q1)
    else:
        r01 = r_tm(q0, q1, e0, e1)

    # Effective reflection seen when standing in layer-1 and looking downward.
    # This already includes all phase effects of the lower multilayer stack.
    rF = RF_multilayer(n_list, d_list, polarization, k0, kp, target_layer=1)

    return (r01 + rF) / (1.0 + r01 * rF)


def r_finite_bounce_single_film(n_list, d_list, polarization, k0, kp, n_bounce):
    """
    Finite-bounce summation for one spacer layer (layer 1) above an arbitrary
    multilayer reflector below it, using only r-relations:
    t01*t10 = 1 - r01^2
    r0 ≈ r01 + (1-r01^2)*rF * sum_{m=0}^{n_bounce-1} (r10*rF)^m
    where rF is obtained from RF_multilayer(..., target_layer=1)
    """
    if len(n_list) < 3 or len(d_list) != len(n_list) - 1:
        raise ValueError("require at least 3 layers and d_list length = len(n_list)-1")
    if n_bounce < 1:
        raise ValueError("n_bounce must be >= 1")

    e0, e1 = n_list[0]**2, n_list[1]**2
    q0 = q_of(e0, k0, kp**2)   # [修正1] 原: np.sqrt(kp**2 - e0 * k0**2 + 0j) / k0
    q1 = q_of(e1, k0, kp**2)   # [修正1] 原: np.sqrt(kp**2 - e1 * k0**2 + 0j) / k0

    if polarization.upper() == "TE":
        r01 = r_te(q0, q1)
        r10 = r_te(q1, q0)
    else:
        r01 = r_tm(q0, q1, e0, e1)
        r10 = r_tm(q1, q0, e1, e0)

    # Effective reflection seen from layer-1 toward all lower layers.
    # No extra phase multiplier is needed here.
    rF = RF_multilayer(n_list, d_list, polarization, k0, kp, target_layer=1)

    ratio = r10 * rF
    first_internal = (1.0 - r01**2) * rF

    total = r01
    term = first_internal
    for _ in range(n_bounce):
        total += term
        term *= ratio
    return total


def main():
    n_list = [1.0, 1.5, 1.3, 1.5, 1.0]  # refractive indices (最後一層是 substrate)
    d_list = [0.0, 500e-9, 500e-9, 500e-9]  # thicknesses (最後一層無厚度)

    wl_min, wl_max, points = 400e-9, 1000e-9, 500
    wl = np.linspace(wl_min, wl_max, points)
    k0 = 2*np.pi / wl
    # [修正2] 原: theta = 30.0  (直接丟進 np.sin，被當成 30 弧度，等效約 81 度)
    theta_deg = 30.0  # incidence angle in degrees (0 = normal incidence)
    theta = np.deg2rad(theta_deg)  # np.sin expects radians
    kp = n_list[0] * k0 * np.sin(theta)
    polarization = "TE"
    finite_layer = [1,2,3]
    n_bounce = [2]
    bounce_list = [1, 2, 5, 10, 20, 40, 80]

    r_RF = np.empty(points, dtype=complex)
    r_RB = np.empty(points, dtype=complex)
    r_HY_RF = np.empty(points, dtype=complex)
    r_HY_RB = np.empty(points, dtype=complex)
    for i in range(points):
        # RF: 從頂層往下
        r_RF[i] = RF_multilayer(n_list, d_list, polarization, k0[i], kp[i], target_layer=0)  # [修正3] 原: "TM"
        # RB: 從底層往上
        r_RB[i] = RB_multilayer(n_list, d_list, polarization, k0[i], kp[i], target_layer=len(n_list)-1)  # [修正3] 原: "TM"
        # Hybrid finite-bounce (RF recursion)
        r_HY_RF[i] = RF_multilayer_hybrid_finite(
            n_list,
            d_list,
            polarization,  # [修正3] 原: "TE"
            k0[i],
            kp[i],
            target_layer=0,
            finite_layer=finite_layer,
            n_bounce=n_bounce,
        )
        # Hybrid finite-bounce (RB recursion)
        r_HY_RB[i] = RB_multilayer_hybrid_finite(
            n_list,
            d_list,
            polarization,  # [修正3] 原: "TE"
            k0[i],
            kp[i],
            target_layer=len(n_list)-1,
            finite_layer=finite_layer,
            n_bounce=n_bounce,
        )

    err_hy_rf = np.abs(r_HY_RF - r_RF)
    err_hy_rb = np.abs(r_HY_RB - r_RB)
    err_rb_rf = np.abs(r_RB - r_RF)
    print(f"Polarization: {polarization}")
    print(f"finite_layer: {finite_layer}")
    print(f"n_bounce: {n_bounce}")
    print(f"max |RB - RF|  = {np.max(err_rb_rf):.3e}")
    print(f"mean|RB - RF|  = {np.mean(err_rb_rf):.3e}")
    print(f"max |HybridRF - RF|  = {np.max(err_hy_rf):.3e}")
    print(f"mean|HybridRF - RF|  = {np.mean(err_hy_rf):.3e}")
    print(f"max |HybridRB - RB|  = {np.max(err_hy_rb):.3e}")
    print(f"mean|HybridRB - RB|  = {np.mean(err_hy_rb):.3e}")

    print("\nConvergence table (RF-hybrid vs RF)")
    print(f"{'n_bounce':>8}  {'max error':>12}  {'mean error':>12}")
    for n_test in bounce_list:
        r_hy_test = np.empty(points, dtype=complex)
        for i in range(points):
            r_hy_test[i] = RF_multilayer_hybrid_finite(
                n_list,
                d_list,
                polarization,
                k0[i],
                kp[i],
                target_layer=0,
                finite_layer=finite_layer,
                n_bounce=n_test,
            )
        err_test = np.abs(r_hy_test - r_RF)
        print(f"{n_test:8d}  {np.max(err_test):12.3e}  {np.mean(err_test):12.3e}")

    print("\nConvergence table (RB-hybrid vs RB)")
    print(f"{'n_bounce':>8}  {'max error':>12}  {'mean error':>12}")
    for n_test in bounce_list:
        r_hy_test = np.empty(points, dtype=complex)
        for i in range(points):
            r_hy_test[i] = RB_multilayer_hybrid_finite(
                n_list,
                d_list,
                polarization,
                k0[i],
                kp[i],
                target_layer=len(n_list)-1,
                finite_layer=finite_layer,
                n_bounce=n_test,
            )
        err_test = np.abs(r_hy_test - r_RB)
        print(f"{n_test:8d}  {np.max(err_test):12.3e}  {np.mean(err_test):12.3e}")

    plt.figure(figsize=(9, 5))
    plt.plot(wl*1e9, np.abs(r_RF), label="|RF|")
    plt.plot(wl*1e9, np.abs(r_RB), label="|RB|")
    plt.plot(wl*1e9, np.abs(r_HY_RF), "--", label="|HybridRF|")
    plt.plot(wl*1e9, np.abs(r_HY_RB), ":", label="|HybridRB|")
    plt.xlabel("Wavelength (nm)")
    plt.ylabel("Magnitude")
    plt.title("RF, RB and hybrid-finite magnitudes")
    plt.grid(True)
    plt.legend()

    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
