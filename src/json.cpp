

#include "json.h"
#include <iostream>
#include <sstream>

using json::value;
using json::int64_value;
using json::real_value;
using json::string_value;
using json::array_value;
using json::object_value;

using json::parser;

int64_value::int64_value()
    : _value(0)
{
}

int64_value::int64_value(int64_t v)
    : _value(v)
{
}

std::ostream & int64_value::stringify(std::ostream & os) const
{
    os << (int64_t)_value;
    return os;
}

real_value::real_value()
    : _value(0)
{
}

real_value::real_value(double v)
    : _value(v)
{
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


std::ostream & string_value::stringify(std::ostream & os) const
{
    os << "\"";
    for (size_t i = 0; i < _value.size(); i++) {
        char c = _value[i];
        if (c == '\\') {
            os << "\\\\";
        }
        else if (c == '"'){
            os << "\\\"";
        }
        else {
            os << c;
        }
    }
    os << "\"";
    return os;
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

value ** array_value::mutable_slot(int el) {

    if (_values.size() <= el) {
        _values.resize(el + 1, NULL);
    }
    return &(_values[el]);
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

            char c = _is.get();
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
        else if (c == '"') {
            int col = _col;
            std::stringstream result;
            bool unescaped = true;
            do {
                c = _is.get();
                if (unescaped && c == '"') {
                    *tok = token(token::STRING, _line, _col);
                    tok->value = new string_value(result.str());
                    _col++;
                    return true;
                }
                else if (unescaped && c == '\\') {
                    unescaped = false;
                }
                else {
                    unescaped = true;
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
    std::cout << "enter parse_elements" << std::endl;
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
                std::cout << "elements: parse failed  at " << _line << ", " << _col << std::endl;
                return false;
            }
            if (!next(&t)) {
                std::cout << "elements: next failed  at " << _line << ", " << _col << std::endl;
                return false;
            }
        }

        has_more = t.kind == token::COMMA;

    } while (has_more);
    std::cout << "properties " << el << std::endl;
    return t.kind == token::RBRACKET;

}
bool parser::parse_properties(object_value * object) {
    bool has_more = false;
    token t;
    do  {

        if (!next(&t)) {
            std::cout << "properties: next failed  at " << _line << ", " << _col << std::endl;
            return false;
        }
        else if (t.kind == token::RBRACE) {
            return true;
        }

        if (t.kind != token::STRING) {
            std::cout << "expected string" << std::endl;
            return false;
        }

        std::string prop = *(t.value->string());

        if (!next(&t)) {
            std::cout << "properties: next failed  at " << _line << ", " << _col << std::endl;
            return false;
        }
        else if (t.kind != token::COL) {
            std::cout << "expected :" << std::endl;
            return false;
        }

        value ** value_store = object->mutable_slot(prop);
        if (!parse(value_store)) {
            std::cout << "properties: parse " << prop <<" failed  at " << _line << ", " << _col << std::endl;
            return false;
        }

        if (!next(&t)) {
            std::cout << "properties: next failed  at " << _line << ", " << _col << std::endl;
            return false;
        }
        has_more = t.kind == token::COMMA;

    } while (has_more);

    return t.kind == token::RBRACE;
}

bool parser::parse(value ** result) {

    token t;
    if (!next(&t)) {
        std::cout << "next failed  at " << _line << ", " << _col << std::endl;
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
        *result = t.value;
        t.value = NULL;
    }
    return true;
}
