#include "libs/catch/include/catch.hpp"
#include "../netHelper.hpp"

TEST_CASE("netHelper_test") {
    /*
     * open socket
     */
    bestsens::jsonNetHelper * socket = new bestsens::jsonNetHelper("localhost", "6450");

    CHECK(socket->get_sockfd() != 0);
    CHECK(socket->is_logged_in() == 0);
    
}
