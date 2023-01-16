#include "test_cbor.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <json.hpp>

#include <argp.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

using json = nlohmann::json;

#include "nanocbor.h"

static int _parse_type(nanocbor_value_t *value, unsigned indent);


struct arguments {
    bool pretty;
    char *input;
};

static struct arguments _args = { true, NULL };


#define MAX_DEPTH 20

static void _print_indent(unsigned indent)
{
    if (_args.pretty) {
        for (unsigned i = 0; i < indent; i++) {
            printf("  ");
        }
    }
}

static void _print_separator(void)
{
    if (_args.pretty) {
        printf("\n");
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_cbor(nanocbor_value_t *it, unsigned indent)
{
    while (!nanocbor_at_end(it)) {
        _print_indent(indent);
        int res = _parse_type(it, indent);

        if (res < 0) {
            printf("Err\n");
            break;
        }

        if (!nanocbor_at_end(it)) {
            printf(", ");
        }
        _print_separator();
    }
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static void _parse_map(nanocbor_value_t *it, unsigned indent)
{
    while (!nanocbor_at_end(it)) {
        _print_indent(indent);
        int res = _parse_type(it, indent);
        printf(": ");
        if (res < 0) {
            printf("Err\n");
            break;
        }
        res = _parse_type(it, indent);
        if (res < 0) {
            printf("Err\n");
            break;
        }
        if (!nanocbor_at_end(it)) {
            printf(", ");
        }
        _print_separator();
    }
}

/* NOLINTNEXTLINE(misc-no-recursion, readability-function-cognitive-complexity) */
static int _parse_type(nanocbor_value_t *value, unsigned indent)
{
    uint8_t type = nanocbor_get_type(value);
    if (indent > MAX_DEPTH) {
        return -2;
    }
    switch (type) {
    case NANOCBOR_TYPE_UINT: {
        uint64_t uint = 0;
        if (nanocbor_get_uint64(value, &uint) >= 0) {
            printf("%" PRIu64, uint);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_NINT: {
        int64_t nint = 0;
        if (nanocbor_get_int64(value, &nint) >= 0) {
            printf("%" PRIi64, nint);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_BSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        if (nanocbor_get_bstr(value, &buf, &len) >= 0 && buf) {
            size_t iter = 0;
            printf("h\'");
            while (iter < len) {
                printf("%.2x", buf[iter]);
                iter++;
            }
            printf("\'");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_TSTR: {
        const uint8_t *buf = NULL;
        size_t len = 0;
        if (nanocbor_get_tstr(value, &buf, &len) >= 0) {
            printf("\"%.*s\"", (int)len, buf);
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_ARR: {
        nanocbor_value_t arr;
        if (nanocbor_enter_array(value, &arr) >= 0) {
            printf("[");
            _print_separator();
            _parse_cbor(&arr, indent + 1);
            nanocbor_leave_container(value, &arr);
            _print_indent(indent);
            printf("]");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_MAP: {
        nanocbor_value_t map;
        if (nanocbor_enter_map(value, &map) >= NANOCBOR_OK) {
            printf("{");
            _print_separator();
            _parse_map(&map, indent + 1);
            nanocbor_leave_container(value, &map);
            _print_indent(indent);
            printf("}");
        }
        else {
            return -1;
        }
    } break;
    case NANOCBOR_TYPE_FLOAT: {
        bool test = false;
        uint8_t simple = 0;
        float fvalue = 0;
        double dvalue = 0;
        if (nanocbor_get_bool(value, &test) >= NANOCBOR_OK) {
            test ? printf("true") : printf("false");
        }
        else if (nanocbor_get_null(value) >= NANOCBOR_OK) {
            printf("null");
        }
        else if (nanocbor_get_undefined(value) >= NANOCBOR_OK) {
            printf("\"undefined\"");
        }
        else if (nanocbor_get_simple(value, &simple) >= NANOCBOR_OK) {
            printf("\"simple(%u)\"", simple);
        }
        else if (nanocbor_get_float(value, &fvalue) >= 0) {
            printf("%f", fvalue);
        }
        else if (nanocbor_get_double(value, &dvalue) >= 0) {
            printf("%f", dvalue);
        }
        else {
            return -1;
        }
        break;
    }
    case NANOCBOR_TYPE_TAG: {
        uint32_t tag = 0;
        if (nanocbor_get_tag(value, &tag) >= NANOCBOR_OK) {
            printf("%" PRIu32 "(", tag);
            _parse_type(value, 0);
            printf(")");
        }
        else {
            return -1;
        }
        break;
    }
    default:
        printf("Unsupported type\n");
        return -1;
    }
    return 1;
}

void test_cbor()
{

    json j = R"([{"type": 0, "version": 0}, { "type": 1}])"_json;
    std::vector<std::uint8_t> v_cbor = json::to_cbor(j);

    printf("\n");
    for (auto c : v_cbor) {
        printf("%x", (int)c);
    }
    printf("\n");

    nanocbor_value_t decoder;
    nanocbor_decoder_init(&decoder, v_cbor.data(), v_cbor.size());

    _parse_cbor(&decoder, 0);

    // Streaming parser
    nanocbor_value_t it;
    nanocbor_decoder_init(&it, v_cbor.data(), v_cbor.size());
//    while (!nanocbor_at_end(&it)) {
        uint8_t type = nanocbor_get_type(&it);

        if (type == NANOCBOR_TYPE_ARR)
        {
            nanocbor_value_t arr;
            if (nanocbor_enter_array(&it, &arr) >= 0) {
                printf("Size of story: %ld\n", arr.remaining);
            }
        }
//    }

/*
    std::ios oldState(nullptr);
    oldState.copyfmt(std::cout);


    for (auto c : v_cbor) {
        std::cout << std::setfill('0')
                  << std::setw(2)
                  << std::uppercase
                  << std::hex
                  << c;
    }

    std::cout.copyfmt(oldState);
    std::cout << std::endl;
    */
}
