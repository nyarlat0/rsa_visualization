class QString;
class QWidget;
class QLineF;
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

int rem(QWidget *w, double factor = 1.0);
int mod_scale(QWidget *w, int scale, double ratio = 1.5);
bool validate_num(const QString &txt);
