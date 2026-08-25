# -*- coding: utf-8 -*-
"""
P0-1 WCharFieldCompare RED/GREEN regression test (Python equivalent)
Algorithmic logic mirrors C++ WCharFieldCompare_Impl byte-level behavior.
"""
import struct, sys

TOTAL=PASS=FAIL=0

def expect_eq(expect, actual, msg):
    global TOTAL, PASS, FAIL
    TOTAL += 1
    if expect == actual:
        PASS += 1; print("  [PASS] %s: expect=%r actual=%r" % (msg, expect, actual))
    else:
        FAIL += 1; print("  [FAIL] %s: expect=%r actual=%r" % (msg, expect, actual))

def byteswap_ushort(v: int) -> int:
    v &= 0xFFFF
    return ((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8)

def ascii_to_be_wchar_buf(ascii_str: str) -> bytes:
    """ASCII string → Big-Endian WCHAR buffer (each char → 2 bytes: 0x00, ord(c))."""
    out = bytearray()
    for c in ascii_str:
        out.append(0x00)           # BE high byte first
        out.append(ord(c) & 0xFF)  # then low
    return bytes(out)

def WCharFieldCompare_Impl(pField: bytes,     # INI ASCII (bytes)
                           pSrcField: bytes,  # CID Big-Endian WCHAR buffer
                           nSize: int,        # sizeof ini char[]
                           Expected: list,    # WORD list (0..65535), length >= nSize+32 (caller pre-filled)
                           charToWCharSucceed: bool,
                           is_fixed: bool) -> int:
    """C++ WCharFieldCompare algorithm byte-level equivalent (Python)."""
    bRet = 0  # ERROR_SUCCESS
    DestLen = 0
    while DestLen < nSize and DestLen < len(pField) and pField[DestLen:DestLen+1] != b'\x00':
        DestLen += 1
    if DestLen > 0:
        if not charToWCharSucceed:
            bRet = -1
            if is_fixed:
                return bRet  # Fixed: IMMEDIATE return, do not touch loop
            # Buggy: fall through to loop anyway, bRet may be overwritten
        else:
            # CharToWChar: ASCII → UTF-16LE WCHAR, write to Expected[0..DestLen-1]
            for k in range(DestLen):
                if k < nSize and k < len(pField):
                    Expected[k] = pField[k] & 0xFF
    loopEnd = DestLen if is_fixed else DestLen * 2
    for i in range(loopEnd):
        # C++: WCHAR beChar = *(WCHAR*)(pSrcField + i*2);  (small-endian read on x86)
        # buf bytes [lowAddr, highAddr] → small-endian WCHAR value = highAddr*256 + lowAddr
        idx = i * 2
        if idx + 1 < len(pSrcField):
            lowAddrByte = pSrcField[idx] & 0xFF
            highAddrByte = pSrcField[idx+1] & 0xFF
        else:
            lowAddrByte = highAddrByte = 0
        beChar = (highAddrByte << 8) | lowAddrByte     # small-endian value interpreted from raw bytes
        leChar = byteswap_ushort(beChar)               # swap → host order
        if leChar != (Expected[i] & 0xFFFF):
            bRet = leChar - (Expected[i] & 0xFFFF)
            # if signed short wrap (when leChar < Expected[i]) produce negative int — same as C++ signed subtraction
            if bRet >= 0x8000:
                bRet -= 0x10000
            break
    return bRet

# ----------------------------------------------------------------------
def ScenarioA_LongPnmMatch(is_fixed: bool, expectedRet: int):
    """Scenario A: 15-char PNM, perfect match, but overflow Expected[i>=16] = 0xDEAD.
    Buggy  : i runs 0..29, hits i=16 leChar==0 vs Expected[16]=0xDEAD → bRet = 0-0xDEAD = -56973 (误杀)
    Fixed  : i runs 0..14,  all match → 0
    """
    nSize = 16
    iniStr = b"ABCDEFGHIJKLMNO"   # 15 ASCII bytes
    # CID BE WCHAR: 15 chars * 2 = 30 bytes of BE 'A'..'O', then zeros
    cidBuf = ascii_to_be_wchar_buf("ABCDEFGHIJKLMNO") + b"\x00" * (512 - 30)
    # Expected buffer: nSize=16 words + 32 words as overflow
    Expected = [0] * (nSize + 32)
    for k in range(nSize, len(Expected)): Expected[k] = 0xDEAD  # poison tail (越界污染)
    r = WCharFieldCompare_Impl(iniStr, cidBuf, nSize, Expected, True, is_fixed)
    expect_eq(expectedRet, r, ("A.fixed PNM(15)匹配 → 0" if is_fixed else "A.buggy PNM(15)匹配被越界误杀 → 0-0xDEAD"))

def ScenarioB_CharToWcharFail(is_fixed: bool, expectedRet: int):
    """Scenario B: CharToWChar fails. Expected[0] poisoned to 1.
    Fixed  : return -1 immediately, no loop
    Buggy  : enters loop, Expected[0]=1, CID[0] LE='A'=0x0041 → bRet=0x41-1=64 (覆盖了 -1)
    """
    nSize = 8
    iniStr = b"ABC"
    cidBuf = ascii_to_be_wchar_buf("ABC") + b"\x00" * 200
    Expected = [0] * (nSize + 32)
    Expected[0] = 1   # poison so first iteration produces bRet != -1
    r = WCharFieldCompare_Impl(iniStr, cidBuf, nSize, Expected, False, is_fixed)
    expect_eq(expectedRet, r, ("B.fixed CharToWChar失败→-1" if is_fixed else "B.buggy CharToWChar失败被循环改写→64"))

def ScenarioC_MatchButCidHasTrailing(is_fixed: bool, expectedRet: int):
    """Scenario C: INI=ABC (DestLen=3, nSize=8), CID has BE WCHAR 'D' at position 3 (after the 3 match chars).
    Buggy  : loop 0..5, i=3 → CID 'D' vs Expected[3]=0 → bRet='D'-0=68 (误杀)
    Fixed  : loop 0..2, all match → 0
    """
    nSize = 8
    iniStr = b"ABC"
    cidBuf = ascii_to_be_wchar_buf("ABC") + ascii_to_be_wchar_buf("D") + b"\x00"*200
    Expected = [0] * (nSize + 32)
    r = WCharFieldCompare_Impl(iniStr, cidBuf, nSize, Expected, True, is_fixed)
    expect_eq(expectedRet, r, ("C.fixed CID尾部多余字符不影响→0" if is_fixed else "C.buggy CID尾部字符被误读→'D'=68"))

def ScenarioD_Mismatch(is_fixed: bool, expectedSign: int):
    """INI 'ABC' vs CID 'ABD' → mismatch. Both versions should return non-zero."""
    nSize = 8
    iniStr = b"ABC"
    cidBuf = ascii_to_be_wchar_buf("ABD") + b"\x00" * 200
    Expected = [0] * (nSize + 32)
    r = WCharFieldCompare_Impl(iniStr, cidBuf, nSize, Expected, True, is_fixed)
    sign = 0 if r == 0 else (1 if r > 0 else -1)
    expect_eq(expectedSign, sign, ("D.fixed ABC≠ABD→非0" if is_fixed else "D.buggy ABC≠ABD→非0"))

def ScenarioE_Match(is_fixed: bool):
    """Short field 'ABC' exact match with no tail data → both 0."""
    nSize = 8
    iniStr = b"ABC"
    cidBuf = ascii_to_be_wchar_buf("ABC") + b"\x00" * 200
    Expected = [0] * (nSize + 32)
    r = WCharFieldCompare_Impl(iniStr, cidBuf, nSize, Expected, True, is_fixed)
    expect_eq(0, r, ("E.fixed 匹配→0" if is_fixed else "E.buggy 短字段匹配碰巧→0"))

# ----------------------------------------------------------------------
def run_buggy() -> int:
    print("\n--- BUGGY version (current code) ---")
    before = FAIL
    # Expected values for BUGGY (per algorithm analysis)
    ScenarioA_LongPnmMatch(False, 0 - 0xDEAD)   # i=16, leChar=0 vs 0xDEAD
    ScenarioB_CharToWcharFail(False, 0x41 - 1)  # 'A'-1=64, overwrite -1
    ScenarioC_MatchButCidHasTrailing(False, ord('D'))  # 'D'-0=68
    ScenarioD_Mismatch(False, 1)                # D-C=1
    ScenarioE_Match(False)
    return FAIL - before

def run_fixed() -> int:
    print("\n--- FIXED version (after minimal patch) ---")
    before = FAIL
    ScenarioA_LongPnmMatch(True, 0)
    ScenarioB_CharToWcharFail(True, -1)
    ScenarioC_MatchButCidHasTrailing(True, 0)
    ScenarioD_Mismatch(True, 1)
    ScenarioE_Match(True)
    return FAIL - before

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv)>=2 else "both"
    print("=== P0-1 WCharFieldCompare regression test (mode=%s, Python impl) ===" % mode)
    buggy_failed = fixed_failed = 0
    if mode in ("buggy", "both"):
        buggy_failed = run_buggy()
    if mode in ("fixed", "both"):
        fixed_failed = run_fixed()

    print("\n=== Summary: total=%d pass=%d fail=%d ===" % (TOTAL, PASS, FAIL))
    if mode == "both":
        red_ok = buggy_failed > 0   # buggy version must FAIL some assertions (RED proof)
        green_ok = fixed_failed == 0 # fixed version must PASS all (GREEN proof)
        print("  RED proof  (buggy  MUST show failures): %s  (buggy_fail_cnt=%d)" %
              ("PASS - test catches bugs" if red_ok else "FAIL - test invalid! buggy didn't fail", buggy_failed))
        print("  GREEN proof(fixed  MUST pass all)    : %s  (fixed_fail_cnt=%d)" %
              ("PASS - patch works" if green_ok else "FAIL - patch broken", fixed_failed))
        if not red_ok:
            print("\n!!! RED-GREEN TEST INVALID: buggy version doesn't fail, check scenario/expect values")
            sys.exit(2)
        if not green_ok:
            print("\n!!! PATCH BROKEN: fixed version still has failures")
            sys.exit(1)
        sys.exit(0)
    else:
        sys.exit(0 if FAIL == 0 else 1)
