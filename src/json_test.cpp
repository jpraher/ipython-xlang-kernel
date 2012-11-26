
#include "json.h"
#include <iostream>
#include <sstream>
#include <cassert>
namespace json {


    void testBuildSimple() {

        // build a simple message

        object_value root;
        root.set_int64("int", 7);
        root.set_real("fl\"oat", 7.0);
        root.set_string("str", "test");
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


        std::string json_test1 = "  {\n \"test\":  127 \n, \"array\": [   [],\n\"a\\\"\\\\a\"]} ";
        std::istringstream is2(json_test1);
        std::cout << is2.str() << std::endl;
        std::cout << json_test1.size() << std::endl;
        // std::cout << "'" << is2.get() << "'" << std::endl;
        json::parser parser2(is2);
        value * root3 = NULL;
        if (parser2.parse(&root3) && root3) {
            std::cout << "parsed root3" << std::endl;
            root3->stringify(std::cout) << std::endl;
        }
        const object_value * o = root3->object();
        assert(o != NULL);
        assert(*o->int64("test") == 127);
        assert(o->array("array") != NULL);
        assert(o->array("array")->length() == 2);
        assert(o->array("array")->string(1) != NULL);
        assert(*o->array("array")->string(1) == "a\"\\a");
    }


}

int main() {
    json::testBuildSimple();
}
