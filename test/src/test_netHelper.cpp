#include "bone_helper/netHelper.hpp"
#include "catch2/catch_all.hpp"

TEST_CASE("netHelper_test") {
    /*
     * open socket
     */
	auto* socket = new bestsens::jsonNetHelper("localhost", "6450");

	CHECK(socket->get_sockfd() != 0);
    CHECK(socket->is_logged_in() == 0);
    
}
