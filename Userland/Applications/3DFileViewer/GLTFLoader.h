/*
 * Copyright (c) 2022, James Mintram <me@jamesrm.com>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefCounted.h>
#include <AK/RefPtr.h>

#include "Mesh.h"
#include "MeshLoader.h"

class GLTFLoader final : public MeshLoader {
public:
    GLTFLoader() = default;
    ~GLTFLoader() override = default;

    RefPtr<Mesh> load(Core::File& file) override;
};
