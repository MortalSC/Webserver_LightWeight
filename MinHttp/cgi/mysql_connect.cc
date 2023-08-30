#include <iostream>
#include "mysql.h"

int main()
{

    // std::cout << "Version : " << mysql_get_client_info() << std::endl;
    MYSQL *conn = mysql_init(nullptr);
    // mysql_set_character_set(conn, "utf8");
    if (nullptr == mysql_real_connect(conn, "127.0.0.1", "http_test", "123456789", "http_test", 3306, nullptr, 0))
    {
        std::cerr << "connect error!" << std::endl;
        if(mysql_errno(conn)){
            std::cerr << mysql_errno(conn) << " : " << mysql_error(conn) << std::endl; 
        }
        return 1;
    }
    std::cerr << "connect success" << std::endl;

    std::string sql = "insert into user (name, password) values (\'李四\', \'12345678\')";
    std::cerr << "query : " << sql << std::endl;
    int ret = mysql_query(conn, sql.c_str());
    std::cerr << "result : " << ret << std::endl;




    mysql_close(conn);
    return 0;
}
