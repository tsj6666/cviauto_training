#include "TestInfo.h"


GVariant* to_variant(const TestInfo& info) {
    return g_variant_new("(bids)",
                         info.bool_param,
                         info.int_param,
                         info.double_param,
                         info.string_param.c_str());
}

void from_variant(GVariant* variant,TestInfo& info) {
    gboolean b;
    gint i;
    gdouble d;
    const gchar* s;
    // 解包结构体
    g_variant_get(variant, "(bids)", &b, &i, &d, &s);

    info.bool_param = b;
    info.int_param = i;
    info.double_param = d;
    info.string_param = s ? std::string(s) : "";
}



std::string calculate_md5(const void* data, size_t len) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((const unsigned char*)data, len, digest);

    char md5string[33];
    for (int i = 0; i < 16; ++i)
        sprintf(&md5string[i * 2], "%02x", (unsigned int)digest[i]);
    return std::string(md5string);
}


std::mutex g_infoMutex;        // 保护g_info
std::mutex g_fileTransferMutex; // 保护文件传输相关变量

GMainLoop *pLoop = NULL;
TestServiceOrgExampleITestService *pSkeleton = NULL;
GDBusConnection *pConnection = NULL;
TestServiceOrgExampleITestService *proxy;

TestInfo g_info;
std::string g_received_filename;
std::string g_expected_md5;
uint32_t g_expected_filesize = 0;
std::vector<uint8_t> g_file_buffer;


gboolean handleSetTestBool(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gboolean param, gpointer user_data) {
    g_print("Server handleSetTestBool is call. bool is : %d.\n", param);
    std::lock_guard<std::mutex> lock(g_infoMutex);
    g_info.bool_param = param;
    test_service_org_example_itest_service_emit_on_test_bool_changed(skeleton, param);
    test_service_org_example_itest_service_complete_set_test_bool(skeleton, inv, TRUE);
    return TRUE;
}

gboolean handleSetTestInt(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gint32 param, gpointer user_data) {
    g_print("Server handleSetTestInt is call. int is : %d.\n", param);
    std::lock_guard<std::mutex> lock(g_infoMutex);
    g_info.int_param = param;
    test_service_org_example_itest_service_emit_on_test_int_changed(skeleton, param);
    test_service_org_example_itest_service_complete_set_test_int(skeleton, inv, TRUE);
    return TRUE;
}

gboolean handleSetTestDouble(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gdouble param, gpointer user_data) {
    g_print("Server handleSetTestDouble is call. double is : %f.\n", param);
    std::lock_guard<std::mutex> lock(g_infoMutex);
    g_info.double_param = param;
    test_service_org_example_itest_service_emit_on_test_double_changed(skeleton, param);
    test_service_org_example_itest_service_complete_set_test_double(skeleton, inv, TRUE);
    return TRUE;
}

gboolean handleSetTestString(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, const gchar *param, gpointer user_data) {
    g_print("Server handleSetTestString is call. string is : %s.\n", param);
    std::lock_guard<std::mutex> lock(g_infoMutex);
    g_info.string_param = param;
    test_service_org_example_itest_service_emit_on_test_string_changed(skeleton, param);
    test_service_org_example_itest_service_complete_set_test_string(skeleton, inv, TRUE);
    return TRUE;
}

gboolean handleSetTestInfo(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv,
                                  GVariant *param, gpointer user_data) {
    std::lock_guard<std::mutex> lock(g_infoMutex);
    from_variant(param, g_info);
    g_print("Server handleSetTestInfo is call. bool_param: %d, int_param: %d, double_param: %f, string_param: %s.\n", 
            g_info.bool_param, g_info.int_param, g_info.double_param, g_info.string_param.c_str());
    test_service_org_example_itest_service_emit_on_test_info_changed(skeleton, param);
    test_service_org_example_itest_service_complete_set_test_info(skeleton, inv, TRUE);
    return TRUE;
}

gboolean handleGetTestBool(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gpointer user_data) {
    gboolean bool_val;
    {
        std::lock_guard<std::mutex> lock(g_infoMutex);
        bool_val = g_info.bool_param;
    }
    g_print("Server handleGetTestBool is call. bool is : %d.\n", bool_val);
    test_service_org_example_itest_service_complete_get_test_bool(skeleton, inv, bool_val);
    return TRUE;
}

gboolean handleGetTestInt(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gpointer user_data) {
    gint32 int_val;
    {
        std::lock_guard<std::mutex> lock(g_infoMutex);
        int_val = g_info.int_param;
    }
    g_print("Server handleGetTestInt is call. Int is : %d.\n", int_val);
    test_service_org_example_itest_service_complete_get_test_int(skeleton, inv, int_val);
    return TRUE;
}

gboolean handleGetTestDouble(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gpointer user_data) {
    gdouble double_val;
    {
        std::lock_guard<std::mutex> lock(g_infoMutex);
        double_val = g_info.double_param;
    }
    g_print("Server handleGetTestDouble is call. Double is : %f.\n", double_val);
    test_service_org_example_itest_service_complete_get_test_double(skeleton, inv, double_val);
    return TRUE;
}

gboolean handleGetTestString(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gpointer user_data) {
    std::string string_val;
    {
        std::lock_guard<std::mutex> lock(g_infoMutex);
        string_val = g_info.string_param;
    }
    g_print("Server handleGetTestString is call. String is : %s.\n", string_val.c_str());
    test_service_org_example_itest_service_complete_get_test_string(skeleton, inv, string_val.c_str());
    return TRUE;
}

gboolean handleGetTestInfo(TestServiceOrgExampleITestService *skeleton, GDBusMethodInvocation *inv, gpointer user_data) {
    TestInfo info_copy;
    {
        std::lock_guard<std::mutex> lock(g_infoMutex);
        info_copy = g_info;
    }
    g_print("Server handleGetTestInfo is call. bool_param: %d, int_param: %d, double_param: %f, string_param: %s.\n", 
            info_copy.bool_param, info_copy.int_param, info_copy.double_param, info_copy.string_param.c_str());
    test_service_org_example_itest_service_complete_get_test_info(skeleton, inv, to_variant(info_copy));
    return TRUE;
}

gboolean handleSendFileMetadata(TestServiceOrgExampleITestService *skeleton,
                                       GDBusMethodInvocation *inv,
                                       const gchar *filename,
                                       guint32 filesize,
                                       const gchar *md5,
                                       gpointer user_data) {
    g_print("Server: Metadata received - file: %s, size: %u, md5: %s\n", filename, filesize, md5);

    std::lock_guard<std::mutex> lock(g_fileTransferMutex);
    g_received_filename = filename;
    g_expected_md5 = md5;
    g_expected_filesize = filesize;
    g_file_buffer.clear();
    g_file_buffer.resize(filesize);  // 预分配缓冲区

    test_service_org_example_itest_service_complete_send_file_metadata(skeleton, inv, TRUE);
    return TRUE;
}


gboolean handleSendFileNotification(TestServiceOrgExampleITestService *skeleton,
                                           GDBusMethodInvocation *inv,
                                           const gchar *shm_name,
                                           guint32 offset,
                                           guint32 size,
                                           gboolean is_last_chunk,
                                           gpointer user_data) {
    g_print("Server: Receiving chunk from shm: %s, offset: %u, size: %u\n", shm_name, offset, size);

    int fd = shm_open(shm_name, O_RDONLY, 0666);
    if (fd < 0) {
        g_print("shm_open failed: %s\n", strerror(errno));
        test_service_org_example_itest_service_complete_send_file_notification(skeleton, inv, FALSE);
        return TRUE;
    }

    void* ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        g_print("mmap failed\n");
        close(fd);
        test_service_org_example_itest_service_complete_send_file_notification(skeleton, inv, FALSE);
        return TRUE;
    }

    {
        std::lock_guard<std::mutex> lock(g_fileTransferMutex);
        memcpy(&g_file_buffer[offset], ptr, size);
    }

    munmap(ptr, size);
    close(fd);

    if (is_last_chunk) {
        std::string received_filename;
        std::string expected_md5;
        std::vector<uint8_t> file_buffer_copy;
        {
            std::lock_guard<std::mutex> lock(g_fileTransferMutex);
            received_filename = g_received_filename;
            expected_md5 = g_expected_md5;
            file_buffer_copy = g_file_buffer;
        }

        // 校验 MD5
        std::string actual_md5 = calculate_md5(file_buffer_copy.data(), file_buffer_copy.size());
        if (actual_md5 != expected_md5) {
            g_print("MD5 mismatch! expected: %s, actual: %s\n", expected_md5.c_str(), actual_md5.c_str());
        } else {
            g_print("File received and MD5 verified. Saving to %s\n", received_filename.c_str());
            FILE* f = fopen(received_filename.c_str(), "wb");
            if (f) {
                fwrite(file_buffer_copy.data(), 1, file_buffer_copy.size(), f);
                fclose(f);
            } else {
                g_print("Failed to open file for writing: %s\n", received_filename.c_str());
            }
        }
    }

    test_service_org_example_itest_service_complete_send_file_notification(skeleton, inv, TRUE);
    return TRUE;
}


void GBusAcquired_Callback(GDBusConnection *connection,
                         const gchar *name,
                         gpointer user_data){
    
    GError *pError = NULL;

    pSkeleton = test_service_org_example_itest_service_skeleton_new();

    // 连接所有方法处理函数
    (void) g_signal_connect(pSkeleton, "handle-set-test-bool", G_CALLBACK(handleSetTestBool), NULL);
    (void) g_signal_connect(pSkeleton, "handle-set-test-int", G_CALLBACK(handleSetTestInt), NULL);
    (void) g_signal_connect(pSkeleton, "handle-set-test-double", G_CALLBACK(handleSetTestDouble), NULL);
    (void) g_signal_connect(pSkeleton, "handle-set-test-string", G_CALLBACK(handleSetTestString), NULL);
    (void) g_signal_connect(pSkeleton, "handle-set-test-info", G_CALLBACK(handleSetTestInfo), NULL);
    
    (void) g_signal_connect(pSkeleton, "handle-get-test-bool", G_CALLBACK(handleGetTestBool), NULL);
    (void) g_signal_connect(pSkeleton, "handle-get-test-int", G_CALLBACK(handleGetTestInt), NULL);
    (void) g_signal_connect(pSkeleton, "handle-get-test-double", G_CALLBACK(handleGetTestDouble), NULL);
    (void) g_signal_connect(pSkeleton, "handle-get-test-string", G_CALLBACK(handleGetTestString), NULL);
    (void) g_signal_connect(pSkeleton, "handle-get-test-info", G_CALLBACK(handleGetTestInfo), NULL);

    (void) g_signal_connect(pSkeleton, "handle-send-file-metadata", G_CALLBACK(handleSendFileMetadata), NULL);
    (void) g_signal_connect(pSkeleton, "handle-send-file-notification", G_CALLBACK(handleSendFileNotification), NULL);

    (void) g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(pSkeleton),
                                                connection,
                                                ORG_EXAMPLE_ITESTSERVICE_OBJECT_PATH,
                                                &pError);

    if(pError == 0){
        g_print("skeleton export successfully. \n");
    }else{
        g_print("Error: Failed to export object. Reason: %s.\n", pError->message);
        g_error_free(pError);
        g_main_loop_quit(pLoop);
        return;
    }
}


void GBusNameAcquired_Callback (GDBusConnection *connection,
                             const gchar *name,
                             gpointer user_data){
    g_print("GBusNameAcquired_Callback, Acquired bus name: %s \n", ORG_EXAMPLE_ITESTSERVICE_NAME);
}

void GBusNameLost_Callback (GDBusConnection *connection,
                         const gchar *name,
                         gpointer user_data){
    if (connection == NULL)
    {
        g_print("GBusNameLost_Callback, Error: Failed to connect to dbus. \n");
    }else{
        g_print("GBusNameLost_Callback, Error: Failed to get dbus name : %s\n", ORG_EXAMPLE_ITESTSERVICE_NAME);
    }
    g_main_loop_quit(pLoop);
}


bool initDBusCommunicationForServer(void){
    
    bool bRet = TRUE;

    pLoop = g_main_loop_new(nullptr, FALSE); 

    guint own_id = 
        g_bus_own_name (ORG_EXAMPLE_ITESTSERVICE_BUS,
                    ORG_EXAMPLE_ITESTSERVICE_NAME,
                    G_BUS_NAME_OWNER_FLAGS_NONE,
                    &GBusAcquired_Callback,
                    &GBusNameAcquired_Callback,
                    &GBusNameLost_Callback,
                    NULL,
                    NULL);

    return bRet;
}

void *run(void* arg)
{
    g_main_loop_run(pLoop);
    return nullptr;
}


//客户端
void handleBoolChanged(TestServiceOrgExampleITestService *proxy, gboolean param, gpointer user_data) {
    std::cout << "[Signal] Bool changed: " << param << std::endl;
}
void handleIntChanged(TestServiceOrgExampleITestService *proxy, gint32 param, gpointer user_data) {
    std::cout << "[Signal] Int changed: " << param << std::endl;
}
void handleDoubleChanged(TestServiceOrgExampleITestService *proxy, gdouble param, gpointer user_data) {
    std::cout << "[Signal] Double changed: " << param << std::endl;
}
void handleStringChanged(TestServiceOrgExampleITestService *proxy, const gchar* param, gpointer user_data) {
    std::cout << "[Signal] String changed: " << param << std::endl;
}
void handleInfoChanged(TestServiceOrgExampleITestService *proxy, GVariant *param, gpointer user_data) {
    gboolean b;
    gint i;
    gdouble d;
    const gchar* s;

    g_variant_get(param, "(bids)", &b, &i, &d, &s);
    g_print("[Signal] Info changed, bool_param:%d, int_param:%d, double_param:%f, string_param:%s.\n", b, i, d, s);
}


bool initDBusCommunicationForClient(){
	
	bool bRet = TRUE;
    GError *pConnError = NULL;
    GError *pProxyError = NULL;
	
	do{
		bRet = TRUE;
		pLoop = g_main_loop_new(NULL,FALSE); 
		
		pConnection = g_bus_get_sync(ORG_EXAMPLE_ITESTSERVICE_BUS, NULL, &pConnError);
		if(pConnError == NULL){
			proxy = test_service_org_example_itest_service_proxy_new_sync(
                    pConnection, G_DBUS_PROXY_FLAGS_NONE,
                    ORG_EXAMPLE_ITESTSERVICE_NAME,        // service name
                    ORG_EXAMPLE_ITESTSERVICE_OBJECT_PATH,       // object path
                    nullptr, &pProxyError);
			if(proxy == 0){
				g_print("initDBusCommunication: Create proxy failed. Reason: %s.\n", pConnError->message);
				g_error_free(pProxyError);
				bRet = FALSE;
			}else{
				g_print("initDBusCommunication: Create proxy successfully. \n");
			}
		}else{
			g_print("initDBusCommunication: Failed to connect to dbus. Reason: %s.\n", pConnError->message);
            g_error_free(pConnError);
            bRet = FALSE;
		}
	}while(bRet == FALSE);
					 
	if(bRet == TRUE){
		g_signal_connect(proxy, "on-test-bool-changed", G_CALLBACK(handleBoolChanged), nullptr);
        g_signal_connect(proxy, "on-test-int-changed", G_CALLBACK(handleIntChanged), nullptr);
        g_signal_connect(proxy, "on-test-double-changed", G_CALLBACK(handleDoubleChanged), nullptr);
        g_signal_connect(proxy, "on-test-string-changed", G_CALLBACK(handleStringChanged), nullptr);
        g_signal_connect(proxy, "on-test-info-changed", G_CALLBACK(handleInfoChanged), nullptr);

	}else{
		g_print("initDBusCommunication: Failed to connect signal.  \n");
	}

	return bRet;
}


void show_menu() {
    std::cout << "\n===== D-Bus Client Menu =====" << std::endl;
    std::cout << "1.  Set Bool (0/1)"           << std::endl;
    std::cout << "2.  Set Int"                  << std::endl;
    std::cout << "3.  Set Double"               << std::endl;
    std::cout << "4.  Set String"               << std::endl;
    std::cout << "5.  Set Info (bool int double string)" << std::endl;
    std::cout << "6.  Get Bool"                 << std::endl;
    std::cout << "7.  Get Int"                  << std::endl;
    std::cout << "8.  Get Double"               << std::endl;
    std::cout << "9.  Get String"               << std::endl;
    std::cout << "10. Get Info"                 << std::endl;
    std::cout << "11. Send File"                << std::endl;
    std::cout << "0.  Exit"                     << std::endl;
    std::cout << "============================" << std::endl;
}

std::string get_basename(const std::string& full_path) {
    char *path_copy = strdup(full_path.c_str());
    std::string result = basename(path_copy);
    free(path_copy);
    return result;
}

bool send_file(const std::string& filename) {
    std::string file_name_only = get_basename(filename);

    // 读取文件
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) {
        std::cerr << "Failed to open file.\n";
        return false;
    }
    fseek(f, 0, SEEK_END);
    size_t filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buffer(filesize);
    fread(buffer.data(), 1, filesize, f);
    fclose(f);

    // 计算 MD5
    std::string md5 = calculate_md5(buffer.data(), filesize);

    // 发送 Metadata
    gboolean meta_ok = test_service_org_example_itest_service_call_send_file_metadata_sync(
        proxy, file_name_only.c_str(), filesize, md5.c_str(), nullptr, nullptr, nullptr);
    if (!meta_ok) {
        std::cerr << "SendFileMetadata failed\n";
        return false;
    }

    const size_t CHUNK_SIZE = 1024;
    const std::string shm_name = "/file_chunk_shm";
    shm_unlink(shm_name.c_str());
    int fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    ftruncate(fd, CHUNK_SIZE);
    void* shm_ptr = mmap(NULL, CHUNK_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);

    for (size_t offset = 0; offset < filesize; offset += CHUNK_SIZE) {
        size_t chunk_size = std::min(CHUNK_SIZE, filesize - offset);
        memcpy(shm_ptr, buffer.data() + offset, chunk_size);

        gboolean chunk_ok = test_service_org_example_itest_service_call_send_file_notification_sync(
            proxy, shm_name.c_str(), offset, chunk_size, (offset + chunk_size == filesize), nullptr, nullptr, nullptr);
        if (!chunk_ok) {
            std::cerr << "SendFileNotification failed at offset " << offset << "\n";
            break;
        }
    }

    munmap(shm_ptr, CHUNK_SIZE);
    close(fd);
    shm_unlink(shm_name.c_str());

    return true;
}

void clientLoop(){
    int choice;
    while (true) {
        usleep(100000);
        show_menu();
        g_print("Choose: ");
        
        // 处理菜单选择输入
        while (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            g_print("Error: Invalid input. Please enter a number.\n");
            g_print("Choose: ");
        }
        
        if (choice == 0) break;

        if (choice == 1) {
            gboolean  val;
            g_print("Input bool (0 or 1): ");
            
            // 处理布尔值输入
            while (!(std::cin >> val)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                g_print("Error: Invalid input. Please enter 0 or 1.\n");
                g_print("Input bool (0 or 1): ");
            }
            
            gboolean ok = test_service_org_example_itest_service_call_set_test_bool_sync(proxy, val, nullptr, nullptr, nullptr);
            g_print("SetTestBool result: %d\n", ok);
        } 
        else if (choice == 2) {
            gint val;
            g_print("Input int: ");
            
            // 处理整数输入
            while (!(std::cin >> val)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                g_print("Error: Invalid input. Please enter an integer.\n");
                g_print("Input int: ");
            }
            
            gboolean ok = test_service_org_example_itest_service_call_set_test_int_sync(proxy, val, nullptr, nullptr, nullptr);
            g_print("SetTestInt result: %d\n", ok);
        } 
        else if (choice == 3) {
            gdouble val;
            g_print("Input double: ");
            
            // 处理双精度浮点数输入
            while (!(std::cin >> val)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                g_print("Error: Invalid input. Please enter a valid number.\n");
                g_print("Input double: ");
            }
            
            gboolean ok = test_service_org_example_itest_service_call_set_test_double_sync(proxy, val, nullptr, nullptr, nullptr);
            g_print("SetTestDouble result: %d\n", ok);
        } 
        else if (choice == 4) {
            std::string val;
            g_print("Input string: ");
            std::cin >> val;
            
            gboolean ok = test_service_org_example_itest_service_call_set_test_string_sync(proxy, val.c_str(), nullptr, nullptr, nullptr);
            g_print("SetTestString result: %d\n", ok);
        } 
        else if (choice == 5) {
            gboolean b; gint i; gdouble d; std::string s;
            g_print("Input bool (0/1), int, double, string: ");
            
            // 处理多个输入值
            while (!(std::cin >> b >> i >> d >> s)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                g_print("Error: Invalid input. Please enter valid values.\n");
                g_print("Input bool (0/1), int, double, string: ");
            }
            
            gboolean ok = test_service_org_example_itest_service_call_set_test_info_sync(proxy, g_variant_new("(bids)", b, i, d, s.c_str()), nullptr, nullptr, nullptr);
            g_print("SetTestInfo result: %d\n", ok);
        } 
        else if (choice == 6) {
            gboolean  ret;
            test_service_org_example_itest_service_call_get_test_bool_sync(proxy, &ret, nullptr, nullptr);
            g_print("GetTestBool: %d\n", ret);
        } 
        else if (choice == 7) {
            gint32 ret;
            test_service_org_example_itest_service_call_get_test_int_sync(proxy, &ret, nullptr, nullptr);
            g_print("GetTestInt: %d\n", ret);
        } 
        else if (choice == 8) {
            gdouble ret;
            test_service_org_example_itest_service_call_get_test_double_sync(proxy, &ret, nullptr, nullptr);
            g_print("GetTestDouble: %f\n", ret);
        } 
        else if (choice == 9) {
            gchar *ret;
            test_service_org_example_itest_service_call_get_test_string_sync(proxy, &ret, nullptr, nullptr);
            g_print("GetTestString: %s\n", ret);
            g_free(ret);
        } 
        else if (choice == 10) {
            gboolean b; gint32 i; gdouble d; gchar *s;
            GVariant* ret;
            test_service_org_example_itest_service_call_get_test_info_sync(proxy, &ret, nullptr, nullptr);
            g_variant_get(ret, "(bids)", &b, &i, &d, &s);
            g_print("GetTestInfo: bool_param:%d, int_param:%d, double_param:%f, string_param:%s.\n", b, i, d, s);
            g_free(s);
        }
        else if (choice == 11) {
            std::string filename;
            g_print("Input file path: ");
            std::cin.ignore(); // 清除缓冲区中的换行符
            std::getline(std::cin, filename);
            
            if (send_file(filename)) {
                g_print("File sent successfully.\n");
            } else {
                g_print("Failed to send file.\n");
            }
        }
        else {
            g_print("Error: Invalid choice. Please enter a number between 0 and 11.\n");
        }
    }

    g_object_unref(proxy);
    g_object_unref(pConnection);
    g_main_loop_unref(pLoop);
}
