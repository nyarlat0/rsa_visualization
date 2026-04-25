#include "rsa.h"
#include <QByteArray>
#include <boost/multiprecision/fwd.hpp>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

namespace rsa {

cpp_int encode_message(const QString &m) {
  auto bytes = m.toUtf8();
  cpp_int result = 0;

  for (unsigned char byte : bytes) {
    result <<= 8;
    result += byte;
  }

  return result;
}

cpp_int encrypt(const QString &plaintext, const cpp_int &e, const cpp_int &n) {

  if (e <= 0)
    throw std::runtime_error("Exponent e should be nonzero!");

  if (n <= 0)
    throw std::runtime_error("Modulus n should be nonzero!");

  auto m = encode_message(plaintext);

  if (m >= n)
    throw std::runtime_error("Message is too big to be encoded with given n!");

  return mod_pow(m, e, n);
}

cpp_int mod_pow(cpp_int a, cpp_int exp, const cpp_int &mod) {

  cpp_int result = 1;
  while (exp > 0) {
    if (exp & 1)
      result = (result * a) % mod;

    a = (a * a) % mod;
    exp >>= 1;
  }

  return result;
}

cpp_int gcd(cpp_int a, cpp_int b) {
  while (b != 0) {
    cpp_int r = a % b;
    a = b;
    b = r;
  }

  return a;
}

cpp_int lcm(const cpp_int &a, const cpp_int &b) { return (a / gcd(a, b)) * b; }

cpp_int extended_gcd(const cpp_int &a, const cpp_int &b, cpp_int &x,
                     cpp_int &y) {
  if (b == 0) {
    x = 1;
    y = 0;
    return a;
  }

  cpp_int x1, y1;
  cpp_int g = extended_gcd(b, a % b, x1, y1);

  x = y1;
  y = x1 - (a / b) * y1;

  return g;
}

cpp_int mod_inverse(const cpp_int &a, const cpp_int &mod) {
  cpp_int x, y;
  cpp_int g = extended_gcd(a, mod, x, y);

  if (g != 1)
    throw std::runtime_error(
        "Value has no modular inverse for the given modulus!");

  x %= mod;
  if (x < 0)
    x += mod;

  return x;
}

QString decrypt(const cpp_int &ciphertext, const cpp_int &d, const cpp_int &p,
                const cpp_int &q) {
  if (p == q)
    throw std::runtime_error("p and q must be distinct primes!");

  if (p <= 1 || q <= 1)
    throw std::runtime_error("p and q must be primes!");

  cpp_int phi = (p - 1) * (q - 1);

  cpp_int n = p * q;

  cpp_int pnum = 1;
  auto exp = d;
  auto cnum = ciphertext;
  while (exp > 0) {
    if (exp & 1)
      pnum = (pnum * cnum) % n;

    cnum = (cnum * cnum) % n;
    exp >>= 1;
  }

  QByteArray bytes;
  while (pnum > 0) {
    unsigned int byte = (pnum & 0xff).convert_to<unsigned int>();
    bytes.prepend(static_cast<char>(byte));
    pnum >>= 8;
  }

  auto result = QString::fromUtf8(bytes);
  return result;
}

QString bn_to_qstring(const BIGNUM *bn) {
  if (!bn)
    return "";

  char *s = BN_bn2dec(bn);
  if (!s)
    throw std::runtime_error("BN_bn2dec failed");

  QString result = QString::fromLatin1(s);
  OPENSSL_free(s);
  return result;
}

void gen_key(QString &e_str, QString &d_str, QString &n_str, QString &p_str,
             QString &q_str) {
  EVP_PKEY *pkey = EVP_RSA_gen(2048);
  if (!pkey)
    throw std::runtime_error("RSA generation failed");

  BIGNUM *n = nullptr;
  BIGNUM *e = nullptr;
  BIGNUM *d = nullptr;
  BIGNUM *p = nullptr;
  BIGNUM *q = nullptr;

  if (!EVP_PKEY_get_bn_param(pkey, "n", &n) ||
      !EVP_PKEY_get_bn_param(pkey, "e", &e) ||
      !EVP_PKEY_get_bn_param(pkey, "d", &d) ||
      !EVP_PKEY_get_bn_param(pkey, "rsa-factor1", &p) ||
      !EVP_PKEY_get_bn_param(pkey, "rsa-factor2", &q)) {

    BN_free(n);
    BN_free(e);
    BN_free(d);
    BN_free(p);
    BN_free(q);
    EVP_PKEY_free(pkey);
    throw std::runtime_error("Failed to extract RSA parts");
  }

  n_str = bn_to_qstring(n);
  e_str = bn_to_qstring(e);
  d_str = bn_to_qstring(d);
  p_str = bn_to_qstring(p);
  q_str = bn_to_qstring(q);

  BN_free(n);
  BN_free(e);
  BN_free(d);
  BN_free(p);
  BN_free(q);
  EVP_PKEY_free(pkey);
}

} // namespace rsa
