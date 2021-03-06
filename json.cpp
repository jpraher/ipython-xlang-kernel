/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */


#include "json.h"
#include <iostream>
#include <sstream>

#include <glog/logging.h>

using json::value;
using json::boolean_value;
using json::int64_value;
using json::real_value;
using json::string_value;
using json::array_value;
using json::object_value;

using json::parser;

const std::string value::EMPTY_STRING = "";

bool *            value::mutable_boolean()     { return NULL; }
const bool *      value::boolean() const       { return NULL; }
int64_t *            value::mutable_int64()     { return NULL; }
const int64_t *      value::int64() const       { return NULL; }
double *             value::mutable_real()      { return NULL; }
const double *       value::real() const        { return NULL; }
std::string *        value::mutable_string()    { return NULL; }
const std::string *  value::string() const      { return NULL; }
array_value *        value::mutable_array()     { return NULL; }
const array_value *  value::array() const       { return NULL; }
object_value *       value::mutable_object()    { return NULL; }
const object_value * value::object() const      { return NULL; }

bool value::contains(const value *  v) const { return (*this) == (*v); }

std::string value::to_str() const  {
    std::ostringstream oss;
    this->stringify(oss);
    return oss.str();
}

boolean_value::boolean_value()
    : _value(false)
{
}

boolean_value::boolean_value(bool v)
    : _value(v)
{
}

std::ostream & boolean_value::stringify(std::ostream & os) const
{
    os << (_value ? "true" : "false");
    return os;
}

value * boolean_value::clone() const {
    return new boolean_value(_value);
}

bool boolean_value::operator == (const value& v) const
{
    if (v.boolean() == NULL) return false;
    return _value == *(v.boolean());
}


int64_value::int64_value()
    : _value(0)
{
}

int64_value::int64_value(int64_t v)
    : _value(v)
{
}

bool int64_value::operator == (const value& v) const
{
    if (v.int64() == NULL) return false;
    return _value == *(v.int64());
}

std::ostream & int64_value::stringify(std::ostream & os) const
{
    os << (int64_t)_value;
    return os;
}

value * int64_value::clone() const {
    return new int64_value(_value);
}

real_value::real_value()
    : _value(0)
{
}

real_value::real_value(double v)
    : _value(v)
{
}

value * real_value::clone() const {
    return new real_value(_value);
}

bool real_value::operator == (const value& v) const
{
    if (v.real() == NULL) return false;
    return _value == *(v.real());
}


std::ostream & real_value::stringify(std::ostream & os) const
{
    os << _value;
    return os;
}

string_value::string_value(const std::string &v)
    : _value(v)
{
}

value * string_value::clone() const {
    return new string_value(_value);
}

bool string_value::operator == (const value& v) const
{
    if (v.string() == NULL) return false;
    return _value == *(v.string());
}


std::ostream & string_value::stringify(std::ostream & os) const
{
    os << "\"";
    for (size_t i = 0; i < _value.size(); i++) {
        char c = _value[i];
        switch (c) {
        case '\\':
            os << "\\\\";
            break;
        case '"':
            os << "\\\"";
        case '\b':
            os << "\\b";
            break;
        case '\f':
            os << "\\f";
            break;
        case '\n':
            os << "\\n";
            break;
        case '\r':
            os << "\\r";
            break;
        case '\t':
            os << "\\t";
            break;
        default:
            os << c;
        }
    }
    os << "\"";
    return os;
}

array_value::array_value() {
}

array_value::array_value(const array_value & that)
    : _values(that._values.size())
{
    for (int i = 0; i < that._values.size(); i++) {
        if (that._values[i] == NULL) {
            _values[i] = NULL;
        }
        else {
            _values[i] = that._values[i]->clone();
        }
    }
}

array_value::~array_value() {
    for (property_vector::iterator it = _values.begin();
         it != _values.end();
         ++it)
    {
        if (*it != NULL) {
            delete *it;
            *it = NULL;
        }
    }
}

value * array_value::_get(int el) const {
    if (el >= 0 && el < _values.size()) {
        return _values[el];
    }
    return NULL;
}

void array_value::_put(int el, value * value) {
    if (_values.size() <= el) {
        _values.resize(el + 1, NULL);
    }
    _values[el] = value;
}

void array_value::set_boolean(int el, bool val)
{
    value * v = _get(el);
    if (v != NULL) {
        if (v->mutable_boolean()) {
            *(v->mutable_boolean()) = val;
            return;
        }
        else {
            delete v;
        }
    }
    _put(el, new boolean_value(val));
}

const bool *   array_value::boolean(int el) const {
    value * v = _get(el);
    if (v != NULL) {
        return v->boolean();
    }
    return NULL;
}


void array_value::set_int64(int el, int64_t val)
{
    value * v = _get(el);
    if (v != NULL) {
        if (v->mutable_int64()) {
            *(v->mutable_int64()) = val;
            return;
        }
        else {
            delete v;
        }
    }
    _put(el, new int64_value(val));
}

const int64_t *   array_value::int64(int el) const {
    value * v = _get(el);
    if (v != NULL) {
        return v->int64();
    }
    return NULL;
}




void array_value::set_real(int el, double val) {
    value * v = _get(el);
    if (v != NULL) {
        if (v->mutable_real()) {
            *(v->mutable_real()) = val;
            return;
        }
        else {
            delete v;
        }
    }
    _put(el, new real_value(val));
}

const double  *    array_value::real(int el) const {
    value * v = _get(el);
    if (v != NULL) {
        return v->real();
    }
    return NULL;
}

void array_value::set_string(int el, const std::string & val) {
    value * v = _get(el);
    if (v != NULL) {
        if (v->mutable_string()) {
            *(v->mutable_string()) = val;
            return;
        }
        else {
            delete v;
        }
    }
    _put(el, new string_value(val));
}

const std::string * array_value::string(int el) const {
    value * v = _get(el);
    if (v != NULL) {
        return v->string();
    }
    return NULL;
}

array_value * array_value::mutable_array(int el) {
    value * v = _get(el);
    array_value * result = NULL;
    if (v != NULL) {
        result = v->mutable_array();
    }

    if (result == NULL) {
        if (v != NULL)
            delete v;
        result = new array_value();
        _put(el, result);
    }
    return result;
}

const array_value * array_value::array(int el) const {
    value * v = _get(el);
    if (v != NULL) {
        return v->array();
    }
    return NULL;
}

object_value * array_value::mutable_object(int el) {
    value * v = _get(el);
    object_value * result = NULL;
    if (v != NULL) {
        result = v->mutable_object();
    }

    if (result == NULL) {
        if (v != NULL)
            delete v;
        result = new object_value();
        _put(el, result);
    }
    return result;
}

const object_value * array_value::object(int el) const {
    value * v = _get(el);
    if (v != NULL) {
        return v->object();
    }
    return NULL;
}

int array_value::length() const {
    return _values.size();
}

value ** array_value::mutable_slot(int el) {

    if (_values.size() <= el) {
        _values.resize(el + 1, NULL);
    }
    return &(_values[el]);
}


value * array_value::clone() const {
    return new array_value(*this);
}


std::ostream & array_value::stringify(std::ostream & os) const {
    os << "[";
    for (std::vector<value*>::const_iterator it = _values.begin();
         it != _values.end();
         it++
         ) {
        if (it !=  _values.begin()) {
            os << ",";
        }
        if ((*it) == NULL) {
            os << "null";
        }
        else {
            (*it)->stringify(os);
        }
    }
    os << "]";
    return os;
}


bool array_value::operator == (const value& v) const
{
    if (v.array() == NULL) return false;
    return _values == v.array()->_values;
}


object_value::object_value() {
}

object_value::object_value(const object_value & that) {
    for (property_map::const_iterator it = that._values.begin();
         it != that._values.end();
         ++it) {
        if (it->second == NULL) {
            _values.insert(std::make_pair(it->first, (value*)NULL ));
        }
        else {
            _values.insert(std::make_pair(it->first, it->second->clone()));
        }
    }
}

object_value::~object_value() {
    for (property_map::iterator it = _values.begin();
         it != _values.end();
         ++it)
    {
        if (it->second != NULL) {
            delete it->second;
            it->second = NULL;
        }
    }
}

void object_value::merge(const object_value& that) {
    for (property_map::const_iterator it = that._values.begin();
         it != that._values.end();
         ++it) {
        property_map::iterator existing = _values.find(it->first);
        if (existing != _values.end()) {
            if (existing->second != NULL) {
                delete existing->second;
            }
            _values.erase(existing);
        }
        if (it->second == NULL) {
            _values.insert(std::make_pair(it->first, (value*)NULL ));
        }
        else {
            _values.insert(std::make_pair(it->first, it->second->clone()));
        }
    }
}


value* object_value::_find(const std::string &name) const {
    property_map::const_iterator it = _values.find(name);
    if (it == _values.end()) {
        return NULL;
    }
    return it->second;
}

bool object_value::_delete(const std::string &name)  {
    return _values.erase(name) > 0;
}


void object_value::set_boolean(const std::string & name, bool val)
{
    json::value* v = _find(name);
    if (v != NULL) {
        if (v->mutable_boolean()) {
            *(v->mutable_boolean()) = val;
            return;
        }
        else {
            _delete(name);
            delete v;
        }
    }
    _values.insert(std::make_pair(name, new boolean_value(val)));
}

const bool * object_value::boolean(const std::string & name) const
{
    value * v = _find(name);
    if (v != NULL)
        return v->boolean();
    return NULL;
}

void object_value::set_int64(const std::string & name, int64_t val)
{
    json::value* v = _find(name);
    if (v != NULL) {
        if (v->mutable_int64()) {
            *(v->mutable_int64()) = val;
            return;
        }
        else {
            _delete(name);
            delete v;
        }
    }
    _values.insert(std::make_pair(name, new int64_value(val)));
}

const int64_t * object_value::int64(const std::string & name) const
{
    value * v = _find(name);
    if (v != NULL)
        return v->int64();
    return NULL;
}

void object_value::set_real(const std::string & name, double val)
{
    value* v = _find(name);
    if (v != NULL) {
        if (v->mutable_real()) {
            *(v->mutable_real()) = val;
            return;
        }
        else {
            _delete(name);
            delete v;
        }
    }
    _values.insert(std::make_pair(name, new real_value(val)));
}

const double  * object_value::real(const std::string & name) const
{
    value * v = _find(name);
    if (v != NULL)
        return v->real();
    return NULL;
}


void object_value::set_string(const std::string & name, const std::string & val)
{
    value* v = _find(name);
    if (v != NULL) {
        if (v->mutable_string()) {
            *(v->mutable_string()) = val;
            return;
        }
        else {
            _delete(name);
            delete v;
        }
    }
    _values.insert(std::make_pair(name, new string_value(val)));
}

const std::string  * object_value::string(const std::string & name) const
{
    value * v = _find(name);
    if (v != NULL)
        return v->string();
    return NULL;
}

array_value * object_value::mutable_array(const std::string & name)
{
    value * v = _find(name);
    if (v != NULL && v->mutable_array() == NULL) {
        _delete(name);
        delete v;
        v = NULL;
    }

    if (v) return v->mutable_array();
    array_value * result = new array_value();
    _values.insert(std::make_pair(name, result));
    return result;
}

const array_value * object_value::array(const std::string & name) const
{
    value * v = _find(name);
    if (v != NULL)
        return v->array();
    return NULL;
}

object_value * object_value::mutable_object(const std::string & name)
{
    value * v = _find(name);
    if (v != NULL && v->mutable_object() == NULL) {
        _delete(name);
        delete v;
        v = NULL;
    }

    if (v) return v->mutable_object();
    object_value * result = new object_value();
    _values.insert(std::make_pair(name, result));
    return result;
}

value ** object_value::mutable_slot(const std::string &name) {
    if (_values.find(name) == _values.end()) {
        _values.insert(std::make_pair(name, (value*)NULL));
    }
    return &(_values[name]);
}

const object_value * object_value::object(const std::string & name) const
{
    value * v = _find(name);
    if (v != NULL)
        return v->object();
    return NULL;
}

value * object_value::clone() const {
    return new object_value(*this);
}

bool object_value::operator == (const value& v) const
{
    if (v.object() == NULL) return false;
    return _values == v.object()->_values;
}


bool object_value::empty() const {
    return _values.empty();
}

object_value::const_iterator object_value::begin() const { return _values.begin(); };
object_value::const_iterator object_value::end() const { return _values.end(); }

bool object_value::contains(const value * rhs) const {
    if (rhs == NULL || rhs->object() == NULL) return false;
    return contains(*(rhs->object()));
}
bool object_value::contains(const object_value & rhs) const {
    for (json::object_value::const_iterator it = rhs.begin();
         it != rhs.end();
         ++it) {
        const_iterator vit = _values.find(it->first);
        if (vit == end()) {
            return false;
        }

        // NULL not undefined
        if (vit->second == NULL && it->second != NULL) return false;

        it->second->contains(vit->second);
    }
    return true;
}



std::ostream & object_value::stringify(std::ostream & os) const {
    os << "{";
    for (std::map<std::string, value*>::const_iterator it = _values.begin();
         it != _values.end();
         it++
         )
    {
        if (it !=  _values.begin()) {
            os << ",";
        }
        string_value key(it->first);
        key.stringify(os);
        os << ":" ;
        if (it->second == NULL) {
            os << "null";
        }
        else {
            it->second->stringify(os);
        }
    }
    os << "}";
    return os;
}


parser::token::token()
    : kind(UNDEFINED), line(0), col(0), value(NULL)
{
}

parser::token::~token()
{
    if (value) delete value;
    value = NULL;
}


parser::token::token(Kind kind, int line, int col)
    : kind(kind), line(line), col(col), value(NULL)
{
}

parser::parser(std::istream & is)
    : _is(is),
      _line(0),
      _col(-1)
{
    _la_successful = _next(&_la);
}

bool parser::next(token * tok) {
    *tok = _la;
    // clear the value
    _la.value = NULL;
    bool successful = _la_successful;
    _la_successful = _next(&_la);
    return successful;
}

bool parser::_next(token * tok) {
    if (tok == NULL) return false;

    if (tok->value) {
        delete tok->value;
        tok->value = NULL;
    }

    // do {

        char c = _is.get();
        _col++;

        while (isspace(c) && _is.good()) {
            if (c == '\n') {
                _line++;
                _col = -1;
            }

            c = _is.get();
            _col++;
        }

        if (!_is.good()) {
            *tok = token(token::EOF_, _line, _col);
            return false;
        }
        else if (c == token::LBRACE || c == token::RBRACE || c == token::LBRACKET || c == token::RBRACKET || c == token::COL || c == token::COMMA) {
            *tok = token((token::Kind)c, _line, _col);
            return true;
        }
        else if (c == 'n') {  // null
            if (_is.get() == 'u' && _is.get() == 'l' && _is.get() == 'l' && _is.good()) {
                *tok = token(token::NIL, _line, _col);
                _col += 3;
                return true;
            }
            // error
            return false;
        }
        else if (c == 't') {  // true
            if (_is.get() == 'r' && _is.get() == 'u' && _is.get() == 'e' && _is.good()) {
                *tok = token(token::BOOLEAN, _line, _col);
                tok->value = new boolean_value(true);
                _col += 3;
                return true;
            }
            // error
            return false;
        }
        else if (c == 'f') {  // true
            if (_is.get() == 'a' && _is.get() == 'l' && _is.get() == 's' &&  _is.get() == 'e' &&_is.good()) {
                *tok = token(token::BOOLEAN, _line, _col);
                tok->value = new boolean_value(false);
                _col += 4;
                return true;
            }
            // error
            return false;
        }
        else if (c == '"') {
            int col = _col;
            std::stringstream result;
            bool escaped = false;
            do {
                c = _is.get();
                if (!escaped && c == '"') {
                    *tok = token(token::STRING, _line, _col);
                    tok->value = new string_value(result.str());
                    _col++;
                    return true;
                }
                else if (!escaped && c == '\\') {
                    escaped = true;
                }
                else if (escaped && c == '"') {
                    result.put('"');
                    escaped = false;
                }
                else if (escaped && c == '\\') {
                    result.put('\\');
                    escaped = false;
                }
                else if (escaped && c == '/') {
                    result.put('/');
                    escaped = false;
                }
                else if (escaped && c == 'b') {
                    result.put('\b');
                    escaped = false;
                }
                else if (escaped && c == 'f') {
                    result.put('\f');
                    escaped = false;
                }
                else if (escaped && c == 'n') {
                    result.put('\n');
                    escaped = false;
                }
                else if (escaped && c == 'r') {
                    result.put('\r');
                    escaped = false;
                }
                else if (escaped && c == 't') {
                    result.put('\t');
                    escaped = false;
                }
                else if (escaped && c == 'u') {
                    LOG(WARNING) << "Unicode character literals are not yet supported";
                    escaped = false;
                    return false;
                }
                else if (escaped) {
                    LOG(WARNING) << "Unexpected token \\" <<  c;
                    return false;
                }
                else {
                    escaped = false;
                    result.put(c);
                }
                _col++;
            } while (_is.good());

            throw std::exception();
        }
        // TODO: special treatment of different radix
        else if (isdigit(c) || c == '.') {
            int col = _col;
            int base = 10;
            std::string str;
            bool is_int = true;

            while (isdigit(c) && _is.good()){
                str += c;
                c = _is.get();
                _col++;
            }
            if (c == '.') {
                is_int = false;
                str += c;
                c = _is.get();
                _col++;
                while (isdigit(c) && _is.good()){
                    str += c;
                    c = _is.get();
                    _col++;
                }
            }
            // here we read past
            _col--;
            _is.unget();
            if (is_int) {
                *tok = token(token::INT, _line, col);
                int64_t value = strtoll(str.c_str(), NULL, base);
                tok->value = new int64_value(value);
                return true;
            }
            else {
                *tok = token(token::REAL, _line, col);
                double value = strtod(str.c_str(), NULL);
                tok->value = new real_value(value);
                return true;
            }
        }
        return false;
}

bool parser::parse_elements(array_value * array) {
    DLOG(INFO) << "enter parse_elements";
    bool has_more = false;
    int el = -1;
    token t;

    do  {
        el++;

        if (_la.kind == token::RBRACKET) {
            next(&t);
            return true;
        }
        else if (_la.kind != token::COMMA) {
            value ** value_store = array->mutable_slot(el);
            if (!parse(value_store)) {
                LOG(WARNING) << "elements: parse failed  at " << _line << ", " << _col;
                return false;
            }
            if (!next(&t)) {
                LOG(WARNING) << "elements: next failed  at " << _line << ", " << _col;
                return false;
            }
            DLOG(INFO) << "array: " <<  array->to_str();
        }

        has_more = t.kind == token::COMMA;

    } while (has_more);
    DLOG(INFO) << "array size: " << el;
    return t.kind == token::RBRACKET;

}
bool parser::parse_properties(object_value * object) {
    bool has_more = false;
    token t;
    do  {

        if (!next(&t)) {
            LOG(WARNING) << "properties: next failed  at " << _line << ", " << _col;
            return false;
        }
        else if (t.kind == token::RBRACE) {
            return true;
        }

        if (t.kind != token::STRING) {
            LOG(WARNING) << "expected string" ;
            return false;
        }

        std::string prop = *(t.value->string());

        if (!next(&t)) {
            LOG(WARNING) << "properties: next failed  at " << _line << ", " << _col;
            return false;
        }
        else if (t.kind != token::COL) {
            LOG(WARNING) << "expected ':'" << ", obtained " << t.kind;
            return false;
        }

        value ** value_store = object->mutable_slot(prop);
        if (!parse(value_store)) {
            LOG(WARNING) << "properties: parse " << prop <<" failed  at " << _line << ", " << _col;
            return false;
        }

        if (!next(&t)) {
            LOG(WARNING) << "properties: next failed  at " << _line << ", " << _col;
            return false;
        }
        has_more = t.kind == token::COMMA;

    } while (has_more);

    return t.kind == token::RBRACE;
}


bool parser::parse(object_value * object) {
    token t;
    if (!next(&t)) {
        LOG(WARNING) << "next failed  at " << _line << ", " << _col;
        return false;
    }
    if (t.kind != token::LBRACE) {
        LOG(WARNING) << "expected {" ;
        return false;
    }
    return parse_properties(object);
}

bool parser::parse(array_value * array) {
    token t;
    if (!next(&t)) {
        LOG(WARNING) << "next failed  at " << _line << ", " << _col;
        return false;
    }
    if (t.kind != token::LBRACKET) {
        LOG(WARNING) << "expected [";
        return false;
    }
    return parse_elements(array);
}


bool parser::parse(value ** result) {

    token t;
    if (!next(&t)) {
        LOG(WARNING) << "next failed  at " << _line << ", " << _col ;
        return false;
    }
    if (t.kind == token::LBRACE) {
        object_value * object = new object_value();
        if (parse_properties(object)) {
            *result  = object;
        }
        else return false;
    }
    else if (t.kind == token::LBRACKET) {
        array_value * array = new array_value();
        if (parse_elements(array)) {
            *result  = array;
        }
        else  return false;
    }
    else {
        // take the value ownership.
        if (t.value) {
            DLOG(INFO) << "parsed value: " << t.value->to_str();
        }
        *result = t.value;
        t.value = NULL;
    }
    return true;
}
