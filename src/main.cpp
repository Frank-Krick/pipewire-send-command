#include <chrono>
#include <thread>

#include "pipewire/context.h"
#include "pipewire/core.h"
#include "pipewire/keys.h"
#include "pipewire/loop.h"
#include "pipewire/main-loop.h"
#include "pipewire/node.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include "spa/buffer/buffer.h"
#include "spa/param/param.h"
#include "spa/pod/iter.h"
#include "spa/pod/pod.h"
#include "spa/support/loop.h"
#include "spa/utils/dict.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <math.h>
#include <ostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

#include <pipewire/filter.h>
#include <pipewire/pipewire.h>

#include <spa/control/control.h>
#include <spa/debug/pod.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <sys/types.h>

#include <spa/debug/pod.h>

#include "utils/node_registry.h"
#include "utils/pod_message_builder.h"

#define PERIOD_NSEC (SPA_NSEC_PER_SEC / 8)

struct port {};

struct data {
  struct pw_main_loop *loop;
  struct pw_filter *filter;
  struct pw_registry *registry;
  struct port *port;
  utils::NodeRegistry node_registry;
};

struct pw_invoke_set_param_data {
  unsigned int target_node_id;
  pw_client *target_node;
  double value;
};

static void registry_event_global(void *data, uint32_t id, uint32_t permissions,
                                  const char *c_type, uint32_t version,
                                  const struct spa_dict *props) {
  struct data *my_data = static_cast<struct data *>(data);

  std::string type(c_type);
  if (type == "PipeWire:Interface:Node") {
    auto client = static_cast<pw_client *>(
        pw_registry_bind(my_data->registry, id, c_type, PW_VERSION_CLIENT, 0));
    my_data->node_registry.add_node(id, client);
    std::cout << "Registry event global" << std::endl
              << "type: " << type << std::endl
              << "id: " << id << std::endl;
  }
}

static void registry_event_global_remove(void *data, uint32_t id) {
  struct data *my_data = static_cast<struct data *>(data);

  my_data->node_registry.delete_node_by_id(id);
}

static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
    .global_remove = registry_event_global_remove,
};

static void do_quit(void *userdata, int signal_number) {
  data *data = static_cast<struct data *>(userdata);
  pw_main_loop_quit(data->loop);
}

int main(int argc, char *argv[]) {
  struct data data = {};

  pw_init(&argc, &argv);

  data.loop = pw_main_loop_new(NULL);

  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

  auto context = pw_context_new(pw_main_loop_get_loop(data.loop), nullptr, 0);
  auto core = pw_context_connect(context, nullptr, 0);
  data.registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
  struct spa_hook registry_listener;
  spa_zero(registry_listener);
  pw_registry_add_listener(data.registry, &registry_listener, &registry_events,
                           &data);

  std::thread message_sender([&data]() {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    unsigned int target_node_id = 82;
    auto target_node = data.node_registry.get_node_by_id(target_node_id);
    if (!target_node) {
      std::cout << "Didn't find target node id: " << target_node_id
                << std::endl;
      return;
    }
    pw_invoke_set_param_data invoke_data{target_node_id, target_node->client,
                                         13.4};

    pw_loop_invoke(
        pw_main_loop_get_loop(data.loop),
        [](struct spa_loop *loop, bool async, u_int32_t seq, const void *data,
           size_t size, void *user_data) {
          const pw_invoke_set_param_data *param_data =
              static_cast<const pw_invoke_set_param_data *>(data);

          u_int8_t buffer[1024];
          auto pod = utils::PodMessageBuilder::build_set_params_message(
              buffer, sizeof(buffer), "Compressor:ratio", param_data->value);

          spa_debug_pod(0, nullptr, pod);

          pw_node_set_param(
              reinterpret_cast<struct pw_node *>(param_data->target_node),
              SPA_PARAM_Props, 0, pod);

          std::cout << std::endl << "set target node parameter." << std::endl;

          return 0;
        },
        0, &invoke_data, sizeof(pw_invoke_set_param_data), true, nullptr);

    std::cout << "Finished sending command" << std::endl;
  });

  pw_main_loop_run(data.loop);
  pw_proxy_destroy((struct pw_proxy *)data.registry);
  pw_core_disconnect(core);
  pw_filter_destroy(data.filter);
  pw_filter_destroy(data.filter);
  pw_main_loop_destroy(data.loop);
  pw_deinit();

  return 0;
}
