#pragma once

#include <QString>
#include <boost/multiprecision/cpp_int.hpp>

namespace rsa {

using boost::multiprecision::cpp_int;

cpp_int encrypt(const QString &plaintext, const cpp_int &e, const cpp_int &n);

QString decrypt(const cpp_int &ciphertext, const cpp_int &d, const cpp_int &p,
                const cpp_int &q);

void gen_key(QString &e_str, QString &d_str, QString &n_str, QString &p_str,
             QString &q_str);

cpp_int lcm(const cpp_int &a, const cpp_int &b);

cpp_int mod_inverse(const cpp_int &a, const cpp_int &mod);

cpp_int mod_pow(const cpp_int &a, const cpp_int &exp, const cpp_int &mod);

} // namespace rsa
