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
#include <cassert>
#include <glog/logging.h>

namespace json {


    void testBuildSimple() {

        // build a simple message

        object_value root;
        root.set_int64("int", 7);
        root.set_real("fl\"oat", 7.0);
        root.set_string("str", "test");
        root.set_boolean("o", true);
        object_value * sub_object = root.mutable_object("obj");
        sub_object->set_int64("int", 7);
        array_value * sub_array = root.mutable_array("test");
        sub_array->set_int64(0,10);
        sub_array->set_int64(3,10);

        std::ostringstream os;
        root.stringify(os);
        std::cout << "JSON: " << os.str() << std::endl;
        value * root2 = NULL;
        std::istringstream is(os.str());
        json::parser parser(is);
        if (parser.parse(&root2) && root2) {
            std::cout << "parsed root2" << std::endl;
            root2->stringify(std::cout) << std::endl;
        }


        std::string json_test1 = "  {\n \"test\":  127 \n, \"array\": [   [],\n \"a\\\"\\\\a\", true]} ";
        std::istringstream is2(json_test1);
        std::cout << is2.str() << std::endl;
        std::cout << json_test1.size() << std::endl;
        // std::cout << "'" << is2.get() << "'" << std::endl;
        json::parser parser2(is2);
        value * root3 = NULL;
        bool parse_success3 = parser2.parse(&root3);
        assert(parse_success3);
        if (parse_success3 && root3) {
            std::cout << "parsed root3" << std::endl;
            root3->stringify(std::cout) << std::endl;
        }
        const object_value * o = root3->object();
        assert(o != NULL);
        assert(*o->int64("test") == 127);
        assert(o->array("array") != NULL);
        assert(o->array("array")->length() == 3);
        assert(o->array("array")->string(1) != NULL);
        assert(*o->array("array")->string(1) == "a\"\\a");
        assert(o->array("array")->boolean(2) != NULL);
        assert(*o->array("array")->boolean(2));

        std::string json_test4 = "{\"user_variables\": [], \"code\": \"f = 1\\n\", \"silent\": false, \"allow_stdin\": true, \"store_history\": true, \"user_expressions\": {\"a\": 1}}";
        std::istringstream is4(json_test4);
        std::cout << is4.str() << std::endl;
        std::cout << json_test4.size() << std::endl;
        json::parser parser4(is4);
        value * root4 = NULL;
        if (parser4.parse(&root4) && root4) {
            std::cout << "parsed root4" << std::endl;
            root4->stringify(std::cout) << std::endl;
        }


        std::string json_test5 = "{\"user_variables\": [], \"code\": \"f = 1\\n\",  \"allow_stdin\": true, \"store_history\": true, \"user_expressions\": {}}";
        std::istringstream is5(json_test5);
        std::cout << is5.str() << std::endl;
        std::cout << json_test5.size() << std::endl;
        json::parser parser5(is5);
        value * root5 = NULL;
        bool	ok = false;
        ok = parser5.parse(&root5) && root5;
        assert(ok);
        std::cout << "parsed root5" << std::endl;
        assert(root5->contains(root5));
        std::cout << "root5 contains root5" << std::endl;
        assert(root4->contains(root5));
        std::cout << "root4 contains root5" << std::endl;
        assert(!root5->contains(root4));
        std::cout << "root5 not contains root4" << std::endl;
        assert(*root5 == *root5);
        std::cout << "root5 == root5" << std::endl;
        assert(!(*root4 == *root5));
        std::cout << "root4 != root5" << std::endl;
    }




}

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    json::testBuildSimple();

}
