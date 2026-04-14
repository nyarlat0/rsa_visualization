class QString;
class QWidget;
class QLineF;
#include <QVector>
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

int rem(QWidget *w, double factor = 1.0);
int mod_scale(QWidget *w, int scale, double ratio = 1.5);
bool validate_num(const QString &txt);
QVector<QLineF> build_circle_lines(const cpp_int &mod, const cpp_int &exp,
                                   int width, int height);
