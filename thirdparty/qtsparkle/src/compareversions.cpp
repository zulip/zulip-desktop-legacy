/* This file is part of qtsparkle.
   Copyright (c) 2010 David Sansome <me@davidsansome.com>

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "compareversions.h"

#include <QStringList>

namespace qtsparkle {

// Note: This code is based on Sparkle's SUStandardVersionComparator by
//       Andy Matuschak.

namespace {

// String characters classification. Valid components of version numbers
// are numbers, period or string fragments ("beta" etc.).
enum CharType {
  Type_Number,
  Type_Period,
  Type_String
};

CharType ClassifyChar(const QChar& c) {
  if ( c == '.' )
    return Type_Period;
  else if (c.isDigit())
    return Type_Number;
  else
    return Type_String;
}

// Split version string into individual components. A component is continuous
// run of characters with the same classification. For example, "1.20rc3" would
// be split into ["1",".","20","rc","3"].
QStringList SplitVersionString(const QString& version) {
  QStringList list;

  if (version.isEmpty())
    return list; // nothing to do here

  QString s;
  const int len = version.length();

  s = version[0];
  CharType prev_type = ClassifyChar(version[0]);

  for ( int i = 1; i < len; i++ ) {
    const QChar c = version.at(i);
    const CharType new_type = ClassifyChar(c);

    if ( prev_type != new_type || prev_type == Type_Period ) {
      // We reached a new segment. Period gets special treatment,
      // because "." always delimiters components in version strings
      // (and so ".." means there's empty component value).
      list << s;
      s = c;
    } else {
      // Add character to current segment and continue.
      s += c;
    }

    prev_type = new_type;
  }

  // Don't forget to add the last part:
  list << s;

  return list;
}

} // anonymous namespace

bool CompareVersions(const QString& left, const QString& right) {
  const QStringList parts_left = SplitVersionString(left);
  const QStringList parts_right = SplitVersionString(right);

  // Compare common length of both version strings.
  const size_t n = qMin(parts_left.size(), parts_right.size());
  for ( size_t i = 0; i < n; i++ ) {
    const QString a = parts_left[i];
    const QString b = parts_right[i];

    const CharType type_a = ClassifyChar(a[0]);
    const CharType type_b = ClassifyChar(b[0]);

    if ( type_a == type_b ) {
      if ( type_a == Type_String ) {
        if (a != b)
          return a < b;
      } else if ( type_a == Type_Number ) {
        const int int_a = a.toInt();
        const int int_b = b.toInt();
        if (int_a != int_b)
          return int_a < int_b;
      }
    } else {
      // components of different types
      if ( type_a != Type_String && type_b == Type_String ) {
          // 1.2.0 > 1.2rc1
          return false;
      } else if ( type_a == Type_String && type_b != Type_String ) {
          // 1.2rc1 < 1.2.0
          return true;
      } else {
          // One is a number and the other is a period. The period
          // is invalid.
          return (type_a == Type_Number) ? false : true;
      }
    }
  }

  // The versions are equal up to the point where they both still have
  // parts. Lets check to see if one is larger than the other.
  if ( parts_left.count() == parts_right.count() )
      return 0; // the two strings are identical

  // Lets get the next part of the larger version string
  // Note that 'n' already holds the index of the part we want.

  bool shorter_result, longer_result;
  CharType missing_part_type; // ('missing' as in "missing in shorter version")

  if ( parts_left.count() > parts_right.count() ) {
    missing_part_type = ClassifyChar(parts_left[n][0]);
    shorter_result = true;
    longer_result = false;
  } else {
    missing_part_type = ClassifyChar(parts_right[n][0]);
    shorter_result = false;
    longer_result = true;
  }

  if ( missing_part_type == Type_String ) {
    // 1.5 > 1.5b3
    return shorter_result;
  } else {
    // 1.5.1 > 1.5
    return longer_result;
  }
}

} // namespace qtsparkle
