#pragma once

#include "spa/param/param.h"
#include "spa/param/props.h"
#include "spa/pod/iter.h"
#include "spa/utils/type.h"
#include <cstddef>
#include <string>

#include <spa/pod/builder.h>
#include <spa/pod/pod.h>

#include <sys/types.h>

namespace utils {

class PodMessageBuilder {
public:
  static spa_pod *build_set_params_message(u_int8_t *buffer, size_t buffer_size,
                                           std::string param_name,
                                           double param_value) {

    struct spa_pod_builder builder;
    spa_pod_builder_init(&builder, buffer, buffer_size);

    struct spa_pod_frame object_frame;
    spa_pod_builder_push_object(&builder, &object_frame, SPA_TYPE_OBJECT_Props,
                                SPA_PARAM_Props);
    spa_pod_builder_prop(&builder, SPA_PROP_params, 0);

    struct spa_pod_frame struct_frame;
    spa_pod_builder_push_struct(&builder, &struct_frame);
    spa_pod_builder_string(&builder, param_name.c_str());
    spa_pod_builder_double(&builder, param_value);
    spa_pod_builder_pop(&builder, &struct_frame);

    return static_cast<spa_pod *>(spa_pod_builder_pop(&builder, &object_frame));
  }
};

} // namespace utils
