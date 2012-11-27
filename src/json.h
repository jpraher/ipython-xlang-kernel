
#ifndef __json__h__
#define __json__h__

#include <map>
#include <vector>
#include <list>
#include <string>


namespace json {

    class array_value ;
    class object_value ;

    // undefined ..
    class value {
    public:
        virtual int64_t * mutable_int64();
        virtual const int64_t * int64() const;
        virtual bool * mutable_boolean();
        virtual const bool * boolean() const;
        virtual double * mutable_real();
        virtual const double * real() const;
        virtual std::string * mutable_string();
        virtual const std::string * string() const;
        virtual array_value * mutable_array();
        virtual const array_value * array() const;
        virtual object_value * mutable_object();
        virtual const object_value * object() const;

        virtual value * clone() const = 0;
        virtual std::ostream & stringify(std::ostream & os) const = 0;
        std::string    to_str() const;
    };

    class int64_value : public value {
        friend class array_value;
        friend class object_value;
    public:
        int64_value();
        int64_value(int64_t v);
    public:
        virtual ~int64_value() {}
        virtual int64_t * mutable_int64()           { return &_value; }
        virtual const int64_t * int64() const       { return &_value; }
        virtual std::ostream & stringify(std::ostream & os) const;
        virtual value * clone() const;
    private:
        int64_t _value;
    };

    class boolean_value : public value {
        friend class array_value;
        friend class object_value;
    public:
        boolean_value();
        boolean_value(bool v);
    public:
        virtual ~boolean_value() {}
        virtual bool * mutable_boolean()           { return &_value; }
        virtual const bool * boolean() const       { return &_value; }
        virtual std::ostream & stringify(std::ostream & os) const;
        virtual value * clone() const;
    private:
        bool _value;
    };


    class real_value : public value {
        friend class array_value;
        friend class object_value;
    public:
        real_value();
        real_value(double v);
    public:
        virtual ~real_value() {}
        virtual double * mutable_real()           { return &_value; }
        virtual const double * real() const       { return &_value; }
        virtual std::ostream & stringify(std::ostream & os) const;
        virtual value * clone() const;
    private:
        double _value;
    };

    class string_value : public value {
        friend class array_value;
        friend class object_value;
    public:
        string_value(const std::string & v);
    public:
        virtual ~string_value() {}
        virtual std::string * mutable_string()           { return &_value; }
        virtual const std::string * string() const       { return &_value; }
        virtual std::ostream & stringify(std::ostream & os) const;
        virtual value * clone() const;
    private:
        std::string _value;
    };


    class array_value : public value {
        friend class object_value;
        typedef std::vector<value*> property_vector;
    public:
        array_value();
        explicit array_value(const array_value & that);
        virtual ~array_value();
        /*
        void set(int el, int64_t value);
        void set(int el, double value);
        void set(int el, const std::string & value);
        */
        void set_boolean(int el, bool value);
        const bool * boolean(int el) const;
        void set_int64(int el, int64_t value);
        const int64_t *   int64(int el) const;
        void set_real(int el, double value);
        const double  *    real(int el) const;
        void set_string(int el, const std::string & value);
        const std::string * string(int el) const;

        array_value * mutable_array(int el);
        const array_value * array(int el) const;
        object_value * mutable_object(int el);
        const object_value * object(int el) const;

        value ** mutable_slot(int el);
        int length() const;

        virtual array_value * mutable_array()       { return this; }
        virtual const array_value * array() const   { return this; }

        virtual std::ostream & stringify(std::ostream & os) const;
        virtual value * clone() const;
    private:
        value * _get(int el) const;
        void _put(int el, value* v);

        std::vector<value*> _values;
    };


    class object_value : public value {
        friend class array_value;
        typedef std::map<std::string, value*> property_map;

    public:
        object_value();
        explicit object_value(const object_value & that);
        virtual ~object_value();

        // elements are nullable, undefined is NULL.
        /*
           void set(const std::string & name, int64_t value);
           void set(const std::string & name, double value);
           void set(const std::string & name, const std::string & value);
        */
        void set_boolean(const std::string & name, bool value);
        const bool * boolean(const std::string & name) const;
        void set_int64(const std::string & name, int64_t value);
        const int64_t * int64(const std::string & name) const;
        void set_real(const std::string & name, double value);
        const double  * real(const std::string & name) const;
        void set_string(const std::string & name, const std::string & value);
        const std::string  * string(const std::string & name) const;
        array_value * mutable_array(const std::string & name);
        const array_value * array(const std::string & name) const;
        object_value * mutable_object(const std::string & name);
        const object_value * object(const std::string & name) const;

        virtual object_value * mutable_object()       { return this; }
        virtual const object_value * object() const   { return this; }

        void merge(const object_value & value);

        value ** mutable_slot(const std::string & value);

        virtual std::ostream & stringify(std::ostream & os) const;
        virtual value * clone() const;
    private:
        value* _find(const std::string & name) const;
        bool _delete(const std::string & name) ;

        std::map<std::string, value*> _values;
    };



    class parser {
    public:
        struct token {
            enum Kind {
                UNDEFINED = 0,
                NIL = 1,
                INT = 2,
                REAL = 3,
                STRING = 4,
                BOOLEAN = 5,
                EOF_ = 6,
                LBRACE = '{',
                RBRACE = '}',
                LBRACKET = '[',
                RBRACKET = ']',
                COMMA = ',',
                COL  = ':'
            };

            token();
            token(Kind kind, int line, int col);
            ~token();

            Kind kind;
            json::value *value;
            int line;
            int col;

        };

        parser(std::istream & is);

        bool _next(parser::token * tok) ;
        bool next(parser::token * tok) ;

        bool parse(value** value);
        bool parse(object_value * object);
        bool parse(array_value * object);

    private:
        bool parse_properties(object_value * object);
        bool parse_elements(array_value * array);

    private:
        std::istream & _is;
        int _line;
        int _col;

        token _la;
        bool _la_successful;
   };
}




#endif
