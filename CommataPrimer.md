# Commata Primer

This document shows how you can handle CSV texts with Commata on some simple
examples. If what you like to know about Commata is not found in this document,
it is recommended to consult [the specification](https://furfurylic.github.io/commata/CommataSpecification.xml).

For simplicity, codes here omits `#include` directive for the standard library
of C++ language.

## Commata is a header-only library

Commata is a header-only library, so your codes that use Commata compile
if you configure your compiler refer Commata's `include` directory as an include path.
You are not likely to be bothered by link errors around Commata.

Incidentally, Commata has its `CMakeLists.txt` in the top directory to add itself as an `INTERFACE` library.
So if your project that uses Commata is built with CMake 3.14 or later,
you can set up Commata as a depended library in `CMakeLists.txt` in your project as follows:

```CMake
include(FetchContent)

FetchContent_Declare(
    commata
    GIT_REPOSITORY https://github.com/furfurylic/commata.git
    GIT_TAG        master       # In fact, a commit hash is better
)

FetchContent_MakeAvailable(commata)

target_link_libraries(your_project PRIVATE commata)
                                # Include path should be set up
                                # to include Commata's 'include' directory
```

To get back to this primer, C++ codes in this document should compile
if you configure your compiler refer Commata's `include` directory as an include path.

## Sample CSV file

In this document, we will use a text file `stars.csv`, whose content is as
follows:

```
Constellation,Name,Apparent magnitude,"Distance, in parsec"
Virgo,Spica,0.97,77
Virgo,Zavijava,3.60,11
Virgo,Porrima,2.74,38
Virgo,Minelauva,3.38,71

Cygnus,Deneb,1.25,
Cygnus,Albireo,3.08,133
Cygnus,Sadr,2.23,560
Cygnus,Fawaris,2.89,51
```

## Loading the CSV file into memory

Commata offers the type `commata::stored_table`, which serves as a type of
loaded images of CSV texts. Here is a sample code to load the contents of
`stars.csv` into a `stored_table` object:

```C++
#include <commata/parse_csv.hpp>
#include <commata/stored_table.hpp>

using commata::make_stored_table_builder;
using commata::parse_csv;
using commata::stored_table;

void stored_table_sample()
{
  // Make an empty stored_table object
  stored_table table;

  // Open stars.csv and load its content into table
  parse_csv(std::ifstream("stars.csv"), make_stored_table_builder(table));

  std::cout << table.size() << std::endl; // will print "9" (not "10")
  std::cout << table[0][3] << std::endl;  // will print "Distance, in parsec"
  std::cout << table[1][1] << std::endl;  // will print "Spica"
  std::cout << table[5][1] << std::endl;  // will print "Deneb"
  std::cout << table[5][3] << std::endl;  // will print "" (an empty string)
}
```

`table[0]` is short for `table.content()[0]`, where `table.content()` is a
reference to a `std::deque<std::vector<commata::stored_value>>` object owned by
`table`. So `table[0][3]` is a reference to a `commata::stored_value` object.

`table.content()` contains all records in `stars.csv` in order.
No records are treated specially, for example, the first record is not
regarded as the header of the CSV.

Each element of `table.content()` represents a record and consists of values of
all of its fields arranged in order. As mentioned above, each value is
represented by a `stored_value` object.

### The value type

A `stored_value` object is a wrapper of a null-terminated range of `char` owned
by a `stored_table` object. So you can easily pass it to C functions:

```C++
std::cout << std::strlen(table[6][1].c_str()) << std::endl;
  // will print the length of "Albireo", which is 7
```

(Please note that this example is somewhat absurd. `stored_value` has `size`
 member function which does the same thing in constant time.)

`stored_value` supports iterators and has comparison operators with
`const char*`, `std::string`, and `stored_value`:

```C++
for (char c : table[0][0]) {
  std::cout << static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
} // will print "CONSTELLATION"
std::cout << std::endl;

std::cout << (table[1][1] == "Spica") << std::endl;
  // table[1][1] is "Spica", so will print "1" or "true" or something like that
std::cout << (table[1][3] < std::to_string(75)) << std::endl;
  // table[1][3] is "77", so will print the result of the lexicographical
  // comparison of "77" and "75"
```

### Editting a loaded table

A `stored_value` object is not read-only.
You can modify and erase some charaters in place:

```C++
table[5][2].erase(3);   // table[5][2] will change from "1.25" to "1.2"
table[5][2][2] = '3';   // table[5][2] will change from "1.2" to "1.3"
```

But you must use help of the `stored_table` object to lengthen a `stored_value`
object:

```C++
table.rewrite_value(table[5][3], "430");
  // table[5][3] changes from "" to "430"
```

You can also modify the structure of a loaded table:

```C++
table.content().pop_front();  // erases the first record

std::stable_sort(table.content().begin(), table.content().end(),
                 [](const auto& record1, const auto& record2) {
                   return record1[1] < record2[1];
                 });      // sorts on the names of the stars

table.content().emplace_front(1, table.import_value("Constellation"));
  // adds a new record which consists of one value "Constellation"
```

### Support of wide characters

Wide characters are supported similarly to narrow characters:

```C++
#include <commata/parse_csv.hpp>
#include <commata/stored_table.hpp>

using commata::make_stored_table_builder;
using commata::parse_csv;
using commata::wstored_table;

void stored_table_sample2()
{
  wstored_table table;
  parse_csv(std::wifstream("stars.csv"), make_stored_table_builder(table));

  std::wcout << (table[1][0] == L"Virgo") << std::endl;
    // will print "1" or "true" or something like that
}
```

Here the type of `table[1][0]` is `commata::wstored_value`, whose objects
represents null-terminated ranges of `wchar_t`.

## One-pass scanning

Commata has facilities to perform one-pass scanning and on-the-fly type
conversion on CSV texts. To process CSV texts in this manner may be less
flexible but can be far more efficient than to process with fully-loaded images
of CSV texts. Here is an example to parse `stars.csv` and extract only the
names and the apparent magnitudes of the stars with those facilities:

```C++
#include <commata/parse_csv.hpp>
#include <commata/table_scanner.hpp>

using commata::table_scanner;
using commata::make_field_translator;
using commata::parse_csv;

void one_pass_scanning_sample()
{
  std::set<std::string> constellation_set;
  std::vector<std::string> names;
  std::deque<double> magnitudes;

  table_scanner scanner(1 /* means that the first one record is a header */);
  scanner.set_field_scanner(0, make_field_translator(constellation_set));
    // sets a body field scanner for field #0 (zero-based)
  scanner.set_field_scanner(1, make_field_translator(names));
    // sets a body field scanner for field #1 (zero-based)
  scanner.set_field_scanner(2, make_field_translator(magnitudes));
    // sets a body field scanner for field #2 (zero-based)

  parse_csv(std::ifstream("stars.csv"), std::move(scanner));

  std::cout << constellation_set.size() << std::endl;
                                                // will print "2"
  std::copy(constellation_set.cbegin(), constellation_set.cend(),
            std::ostream_iterator<std::string>(std::cout, ' '));
                                                // will print "Cygnus Virgo "
                                                // or something like that
  std::cout << std::endl;
  std::cout << names.size() << std::endl;       // will print "8"
  std::cout << names.back() << std::endl;       // will print "Fawaris"
  std::cout << magnitudes.size() << std::endl;  // will print "8"
  std::cout << magnitudes[2] << std::endl;      // will print "2.74"
                                                // or something like that
}
```

`make_field_translator` makes a _body field scanner_ object which transfers
the translated field value to its argument.

The argument of it can be either of:
 - an lvalue to a non-const container object at the right position or the back
   of which the translated field value is inserted,
 - an output iterator object which receives the translated field value, or
 - a function object which receives the translated field value as its one and
   only parameter.

In the last two cases, the type to which the field values are translated
must be specified explicitly as the first template parameter of
`make_field_translator`. See the following sample:

```C++
#include <commata/parse_csv.hpp>
#include <commata/table_scanner.hpp>

using commata::table_scanner;
using commata::make_field_translator;
using commata::parse_csv;

void one_pass_scanning_sample2()
{
  table_scanner scanner(1);

  // The body field scanner for field #1 will print the name of stars to the
  // standard output
  scanner.set_field_scanner(1,
    make_field_translator<std::string>(     // <std::string> is required
      std::ostream_iterator<std::string>(std::cout, "\n")));

  // The body field scanner for field #2 will make max_magnitude the magnitude
  // of the brightest star (note that brighter stars have smaller magnitudes)
  double max_magnitude = std::numeric_limits<double>::max();
  scanner.set_field_scanner(2,
    make_field_translator<double>(          // <double> is required
      [&max_magnitude] (double magnitude) {
        max_magnitude = std::min(max_magnitude, magnitude);
      }));

  parse_csv(std::ifstream("stars.csv"), std::move(scanner));
    // Here the body field scanner for field #1 will print the names of stars
    // to the standard output on the fly

  std::cout << max_magnitude << std::endl;  // will print "0.97"
                                            // or something like that
}
```

`make_field_translator` makes a body field scanner that translates the field
values into the following types:
 - arithmetic types: `char`, `signed char`, `unsigned char`, `short`,
   `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`,
   `long long`, `unsigned long long`, `float`, `double`, `long double`,
 - standard string types: `std::basic_string<Ch, Tr, Allocator>` where `Ch` is
   the identical type to the character type of the input text, and
 - standard string view types: `std::basic_string_view<Ch, Tr>` where `Ch` is
   the identical type to the character type of the input text.

To translate strings into arithmetic values, Commata employs C function
`std::strtol` and its comrades declared in `<cstdlib>`.

### Installing body field scanners lazily referencing the values of the header fields

`table_scanner` can be configured to scan some first records as header records
and can be set body field scanners lazily. See the following (somewhat lengthy)
example:

```C++
#include <commata/parse_csv.hpp>
#include <commata/table_scanner.hpp>

using commata::table_scanner;
using commata::make_field_translator;
using commata::parse_csv;

void one_pass_scanning_sample3()
{
  std::vector<std::string> names;

  table_scanner scanner(
    [&names, names_attached = false]
    (std::size_t field_index, const std::pair<char*, char*>* field_value,
     table_scanner& scanner) mutable {
      if (field_value) {
        // The value of field_index-th (zero-based) header field is notified,
        // whose value is [field_value.first, field_value.second), and
        // field_value.second is dereferenceable and points the terminating
        // zero
        if (std::string_view(field_value->first,
                             field_value->second - field_value->first)
            == "Name") {
          scanner.set_field_scanner(
            field_index, make_field_translator(names));
          names_attached = true;
        } else {
          return true;  // true to instruct the table scanner to continue to
                        // report the header fields
        }
      } else {
        // An end of a header record is notified
        if (!names_attached) {
          throw std::runtime_error(
            "Cannot find a field of the names of the stars");
        }
      }
      return false; // false to tell the scanner to uninstall this header field
                    // scanner, and tell it that here the header records end
                    // and the next record will be the first body record
    });

  parse_csv(std::ifstream("stars.csv"), std::move(scanner));

  std::cout << names.front() << std::endl;  // will print "Spica"
  std::cout << names[4] << std::endl;       // will print "Deneb"
}
```

As above, you can construct a `table_scanner` object with a three-parameter
function object, which is called as a _header field scanner_.
With this constructor, the constructed `table_scanner` object initially owns
a copy of the specified header field scanner installed, and invoke it on
every header field and every end of every header record.
The header field scanner is unstalled from the `table_scanner` object just
after it returns `false`.

## Errors

### Parse errors

Suppose that `stars2.csv` is like this:

```
Constellation,Name,Apparent magnitude,"Distance, in parsec"
Virgo,Spica,0.97,77"
Virgo,Zavijava,3.60,11
Virgo,Porrima,2.74,38
Virgo,Minelauva,3.38,71

Cygnus,Deneb,1.25,
Cygnus,Albireo,3.08,133
Cygnus,Sadr,2.23,560
Cygnus,Fawaris,2.89,51
```

This is not a well-formed CSV text because the second line (one-based) has a
double quote that voilates the CSV format.

If you call `parse_csv` with a stream with this content, it will throw an
exception.

Exception objects thrown by Commata's implementation because of the content of
the text have types that are or are derived from `commata::text_error`.
So you can try to parse the text by codes like this:

```C++
#include <commata/parse_csv.hpp>
#include <commata/stored_table.hpp>
#include <commata/text_error.hpp>

using commata::make_stored_table_builder;
using commata::parse_csv;
using commata::stored_table;
using commata::text_error;

void stored_table_error_sample()
{
  stored_table table;

  try {
    parse_csv(std::ifstream("stars2.csv"), make_stored_table_builder(table));
  } catch (const text_error& e) {
    std::cout << e.what() << std::endl;
    throw;
  }
}
```

and can get a clue about what was wrong:

```
A quotation mark found in a non-escaped value
```

`text_error` has a member function `info` that returns an object of
`commata::text_error_info`, which possibly have a useful location information
of the error.

So you can write instead like this:

```C++
#include <commata/parse_csv.hpp>
#include <commata/stored_table.hpp>
#include <commata/text_error.hpp>

using commata::make_stored_table_builder;
using commata::parse_csv;
using commata::stored_table;
using commata::text_error;

void stored_table_error_sample2()
{
  stored_table table;

  try {
    parse_csv(std::ifstream("stars2.csv"), make_stored_table_builder(table));
  } catch (const text_error& e) {
    std::cout << e.info() << std::endl;
    throw;
  }
}
```

and might be able to get a clue about what was wrong:

```
A quotation mark found in a non-escaped value; line 2 column 20
```

Note that line indices and column indices in string representations of
`text_error_info` objects are one-based.

Class `text_error` and `text_error_info` are defined in the header
`"commata/text_error.hpp"`.

### Conversion errors in one-pass scanning

Suppose that you would like to get the average distance of the stars in
`stars.csv`. Then you might write codes like these:

```C++
#include <commata/parse_csv.hpp>
#include <commata/table_scanner.hpp>

using commata::table_scanner;
using commata::make_field_translator;
using commata::parse_csv;

void one_pass_scanning_error_sample()
{
  double distance_sum = 0.0;
  std::size_t distance_num = 0;

  table_scanner scanner(1);
  scanner.set_field_scanner(3,
    make_field_translator<double>(
      [&distance_sum, &distance_num](double distance) {
        distance_sum += distance;
        ++distance_num;
      }));

  parse_csv(std::ifstream("stars.csv"), std::move(scanner));

  std::cout << distance_sum / distance_num << std::endl;
    // will print the average distance
}
```

But when you call this function, it will exit with a `text_error` exception
object whose `info()` is likely to tell:

```
Cannot convert an empty string to an instance of double; line 7 column 19
```

This is because the field value for the distance of Deneb is an empty string,
which is not able to be converted into a `double` value.

This behaviour is designed, but if what you want to do is to calculate the
average of the explicitly specified values of distance, in other words,
if you want to ignore any occurrences of empty field values, you can do this:

```C++
#include <commata/parse_csv.hpp>
#include <commata/table_scanner.hpp>

using commata::table_scanner;
using commata::make_field_translator;
using commata::parse_csv;
using commata::replacement_ignore;
using commata::replace_if_skipped;

void one_pass_scanning_error_sample2()
{
  double distance_sum = 0.0;
  std::size_t distance_num = 0;

  table_scanner scanner(1);
  scanner.set_field_scanner(3,
    make_field_translator<double>(
      [&distance_sum, &distance_num](double distance) {
        distance_sum += distance;
        ++distance_num;
      },
      replace_if_skipped<double>(replacement_ignore),               /* added */
      replace_if_conversion_failed<double>(replacement_ignore)));   /* added */

  parse_csv(std::ifstream("stars.csv"), std::move(scanner));

  std::cout << distance_sum / distance_num << std::endl;
    // will print the average distance
}
```

The argument
`replace_if_skipped<double>(replacement_ignore)`
instructs the body field scanner to ignore every case that a record contains
too few fields to reach the field (in this case, third field
(zero-based)); but it is irrelevant to the above-mentioned exception. (Note
that the second field is followed by a comma, which makes the third field
exist.)

On the other hand, the argument
`replace_if_conversion_failed<double>(replacement_ignore)`
instructs the body field scanner to ignore every case that a value of the field
is an empty string; it is to solve the problem we have faced.

Class templates `replace_if_skipped` and `replace_if_conversion_failed` are
defined in the header `"commata/table_scanner.hpp"`.

## Making your own table handler types

So far, the second arguments to `parse_csv` were the return value of
`make_stored_table_builder` or rvalues of `table_scanner` objects. But you can
define your own types whose objects can be passed to `parse_csv`.
These types are called _table handler_ types.

For example, if you want to make a vector of vectors of field values,
the following codes will do:

```C++
#include <commata/parse_csv.hpp>

using commata::parse_csv;

// A table handler type whose objects make a vector of vectors of field values
class vov_table_handler    // vov means "vector of vector"
{
  std::vector<std::vector<std::string>>* records_;
  std::string current_value_;

public:
  using char_type = char;

  explicit vov_table_handler(
    std::vector<std::vector<std::string>>& records) :
    records_(&records)
  {}

  void start_record(const char*)
  {
    records_->emplace_back();
  }

  void update(const char* first, const char* last)
  {
    // Append [first, last) to the current field value
    current_value_.append(first, last);
  }

  void finalize(const char* first, const char* last)
  {
    // Append [first, last) to the current field value
    // as the final chunk of the value
    current_value_.append(first, last);
    records_->back().push_back(std::move(current_value_));
    current_value_.clear();
  }

  void end_record(const char*)
  {}
};

void vov_table_handler_sample()
{
  std::vector<std::vector<std::string>> records;

  parse_csv(std::ifstream("stars.csv"), vov_table_handler(records));

  std::cout << records.size() << std::endl; // will print "9"
  std::cout << records[3][1] << std::endl;  // will print "Porrima"
}
```

A table handler type must have four member functions: `start_record`,
`end_record`, `update`, `finalize`. In addition, it must have a nested type
`char_type`.
If these requirements meet, `parse_csv` emits parsing events to the text
handler objects.

Please note that a field value may be notified to the table handler object as
chunked; not-the-final chunks are notified by `update` and the final chunk is
notified by `finalize`.

(Note that this sample can be suboptimal in terms of performance. To use
`stored_table` and `make_stored_table_builder` seems to be the best to load the
whole contents of CSV texts into memory. But if you want to do is to make a
container object of container object of `std::string` objects (not
`stored_value`objects) each of which represents its corresponding field value,
this sample codes can be faster and optimal.)

`parse_csv` depletes table handler objects passed to it; in other words,
it leaves them in their moved-from states. If your table handler objects has
states and you want to access their not-moved-from states after parsing,
you can pass them wrapping by `std::ref`.

## Empty lines

As astute readers might have noticed, `stars.csv` contains one empty line that
looks like ignored completely by `parse_csv`.

In fact, it is table handlers (the second arguments to `parse_csv`) that ignored
the empty line. To be precise, `parse_csv` reports empty lines to the text
handlers with `empty_physical_line` function if they have it, but all text
handlers that we have mentioned so far do not have it, so `parse_csv` can not
report the empty line to the table handlers.

(Just to be sure, empty lines are reported with `empty_physical_line` only when
they do not belong to any fields. `parse_csv` recognizes that quoted fields
might contain empty lines as constituents of their values.)

If you want your table handlers to regard an empty line as a record that
contains no fields, you can wrap your table handlers with
`make_empty_physical_line_aware`:

```C++
#include <commata/parse_csv.hpp>
#include <commata/stored_table.hpp>
#include <commata/wrapper_handlers.hpp>

using commata::make_empty_physical_line_aware;
using commata::make_stored_table_builder;
using commata::parse_csv;
using commata::stored_table;

void make_empty_physical_line_aware_sample()
{
  stored_table table;

  parse_csv(std::ifstream("stars.csv"),
    make_empty_physical_line_aware(
      make_stored_table_builder(table)));

  std::cout << table.size() << std::endl;     // will print "10" (not "9")
  std::cout << table[5].size() << std::endl;  // will print "0"
  std::cout << table[6][1] << std::endl;      // will print "Deneb"
}
```

## Pull parsing

Commata also has facilities to perform 'pull parsing', in which users can access the
result of parsing in a step-by-step manner.

In pull parsing, the user have a 'cursor' on the CSV text. He/she can read from
the point where the cursor placed, and move the cursor forward.

See the following sample:

```C++
#include <commata/parse_csv.hpp>
#include <commata/table_pull.hpp>

using commata::make_csv_source;
using commata::make_table_pull;

void pull_parsing_sample()
{
  auto p = make_table_pull(make_csv_source(std::ifstream("stars.csv")));
  p.set_empty_physical_line_aware();

  // Skip the first record
  p.skip_record();

  // Skip one field and point the next field
  p(1);

  std::cout << *p << std::endl;                     // will print "Spica"
  std::cout << p->size() << std::endl;              // will print "Spica"'s
                                                    // length
  std::cout << *p.skip_record(5)(1) << std::endl;   // will print "Albireo"
}
```

`make_table_pull` in the sample above returns an object of `table_pull`.
It can proceed to an end of a record by `skip_record` or to a further field by
`operator()`. They take one parameter which instructs the number of ends of records
or fields jumped over. It defaults to `0`. Note that `operator()` cannot make the
`table_pull` object jump over an end of a record.

Also note that `skip_record` and `operator()` of a `table_pull` object return
a reference to the `table_pull` object itself.

The value of the current field where a `table_pull` object points can be got as
a string view object with `table_pull`'s dereference operators `*` and `->`.
Additionally, `table_pull`' offers `c_str` member function that returns a pointer
to a null-terminated sequence suitable for C APIs.

An object of `table_pull` is convertible to `bool`.
It is converted to `false` if it does not point either an end of a record or
a field&mdash;for example, it has reached the EOF.

So you can easily make it 'run to the end' like this:

```C++
#include <commata/parse_csv.hpp>
#include <commata/table_pull.hpp>

using commata::make_csv_source;
using commata::make_table_pull;

void pull_parsing_sample2()
{
  auto p = make_table_pull(make_csv_source(std::ifstream("stars.csv")));
  std::vector<std::string> v;

  while (p.skip_record()(1)) {      // moves to the end of the record and
                                    // skip the first field of the next record
    v.push_back(std::string(*p));
  }

  std::cout << v[0] << std::endl;   // will print "Spica"
  std::cout << v[1] << std::endl;   // will print "Zavijava"
  std::cout << v[7] << std::endl;   // will print "Fawaris"
}
```

In the sample above, please note there is no `p.set_empty_physical_line_aware()`.
By default, `table_pull` objects do not regard empty lines as records with no fields.
Instead, they simply ignore these lines.

In addition, with `state` member function, you can make a `table_pull` object tell its
status, for example, 'points an end of a record', 'points a field', 'reached the EOF',
and so on. This functionality is essential to handle texts whose structure is not known
in advance.
