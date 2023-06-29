#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pwd.h>
#include <time.h>

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

#include <sgx_uswitchless.h>

#include "table.hpp"
#include "preprocess.hpp"

// Initialization for input tables

Table<TableEntry> *tables[3];

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
    {
        SGX_ERROR_NDEBUG_ENCLAVE,
        "The enclave is signed as product enclave, and can not be created as debuggable enclave.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
        printf("Error: Unexpected error occurred.\n");
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
sgx_uswitchless_config_t us_config = SGX_USWITCHLESS_CONFIG_INITIALIZER;

int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    const void *enclave_ex_p[32] = {0};
    
    us_config.num_uworkers = 1;
    us_config.num_tworkers = 1;

    enclave_ex_p[SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX] = (const void *)&us_config;
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave_ex(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL, SGX_CREATE_ENCLAVE_EX_SWITCHLESS, enclave_ex_p);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}

void OcallRead(int64_t index, int *buffer, int size, int tableId, int addrId) {
  tables[tableId]->read(index, size, (TableEntry*)buffer, addrId);
}

void OcallWrite(int64_t index, int *buffer, int size, int tableId, int addrId) {
  tables[tableId]->write(index, size, (TableEntry*)buffer, addrId);
}

void OcallResize(int tableId, int64_t new_size, int addrId) {
  tables[tableId]->resize(new_size, addrId);
}


/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);


    /* Initialize the enclave */
    if(initialize_enclave() < 0){
        printf("Enter a character before exit ...\n");
        getchar();
        return -1; 
    }
    
    /* Utilize trusted libraries */
    ecall_libc_functions();
    ecall_libcxx_functions();
    ecall_thread_functions();

    /* Main code */
    /* Initialization & Preprocess */
    int64_t n0, n1, N, M;
    int B;
    int tableId = 0, tableId0 = 1, tableId1 = 2;
    int tableIds[3] = {tableId, tableId0, tableId1};
    parseTables(tables, n0, n1);
    /* Join process */
    clock_t program_start = clock();
    callEnclave(global_eid, tableIds, n0, n1, N, M, B);
    printf("Total runtime: %.2fs\n", (clock() - program_start) / (float)CLOCKS_PER_SEC);    
    
    printf("Info: Destroying the enclave.\n");

    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);
    
    printf("Info: Enclave successfully returned.\n");
    return 0;
}

