
#include "json.h"
#include <iostream>
#include <sstream>

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
    }


}

int main() {
    json::testBuildSimple();
}
