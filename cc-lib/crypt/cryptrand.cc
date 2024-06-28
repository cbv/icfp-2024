
#include "cryptrand.h"
#include "base/logging.h"
#include "timer.h"


// TODO: In the future, std::random_device may be a good way to avoid
// the differences per platform. But note that as of early 2019, this
// is actually a *deterministic* sequence on mingw-x64!

#if defined(__MINGW32__) || defined(__MINGW64__)
#  include <stdio.h>
#  include <windows.h>
#  include <wincrypt.h>

BOOLEAN RtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
);

// TODO: Other platforms may need to keep state, in which case this
// maybe needs Create() and private implementation. (Even on Windows
// it's dumb for us to keep creating crypto contexts.)
CryptRand::CryptRand() {}

uint64_t CryptRand::Word64() {
  HCRYPTPROV hCryptProv;

  // TODO: I guess I need to port to the "Cryptography Next Generation
  // APIs" at some point.
  //
  // If you have a problem here, it may be because this function tries
  // to load the user's private keys in some situations. (I got mysterious
  // errors after restoring a backup, and had to delete
  //  c:\users\me\appdata\roaming\microsoft\crypto, which was then in
  // the wrong format or something.) CRYPT_VERIFYCONTEXT should prevent
  // it from having to read these keys, though.
  if (!CryptAcquireContext(
          &hCryptProv,
          NULL,
          NULL,
          PROV_RSA_FULL,
          CRYPT_VERIFYCONTEXT)) {
    // Get error message if we can't create the crypto context.
    const DWORD dwError = GetLastError();
    LPSTR lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);
    std::string err = lpMsgBuf;
    LocalFree(lpMsgBuf);
    LOG(FATAL) << err;
  }

  uint64_t data = 0ULL;
  CHECK(CryptGenRandom(hCryptProv, sizeof (data), (uint8_t*)&data));

  if (hCryptProv) {
    CHECK(CryptReleaseContext(hCryptProv, 0));
  }

  return data;

  /*
  uint64_t data;
  CHECK(RtlGenRandom((void*)&data, sizeof (data)));
  return data;
  */
}

#else

CryptRand::CryptRand() {}

uint64_t CryptRand::Word64() {
  FILE *f = fopen("/dev/urandom", "rb");
  CHECK(f) << "/dev/urandom not available?";
  uint64_t data = 0ULL;
  for (int i = 0; i < 8; i++) {
    int c = fgetc(f);
    CHECK(c != EOF);
    data <<= 8;
    data |= (c & 0xFF);
  }

  fclose(f);
  return data;
}

#endif

uint8_t CryptRand::Byte() {
  return Word64() & 0xFF;
}
