/*
 * Copyright (c) 2022, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "GLTFLoader.h"

#include <LibCore/File.h>

#include <AK/JsonParser.h>

RefPtr<Mesh> GLTFLoader::load(Core::File& file)
{
    dbgln("GLTF: Try to load as GLTF");

    auto parsed_result = JsonParser(file.read_all()).parse();

    if (parsed_result.is_error()) {
        dbgln("GLTF: Error parsing JSON {}", parsed_result.error());
        return nullptr;
    }

    dbgln("GLTF: JSON loaded successfuly");

    return nullptr;
}
