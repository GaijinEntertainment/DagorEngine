// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_test_macros.hpp>
#include <math/random/dag_random.h>

enum class Message
{
  Original,
  Corrupted,
};

TEST_CASE("Hijacker appears", "[name resolution]")
{
  TestRuntime testRuntime{};

  Message expectedMessage;

  // /somewhere/message is Original message
  dafg::NodeHandle senderHandle = dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("message", dafg::History::No).blob(Message::Original);
  });

  dafg::NodeHandle receiverHandle =
    (dafg::root() / "somewhere").registerNode("receiver", DAFG_PP_NODE_SRC, [&expectedMessage](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto messageHandle = registry.read("message").blob<Message>().handle();
      return [&expectedMessage, messageHandle] { CHECK(*messageHandle.get() == expectedMessage); };
    });

  {
    expectedMessage = Message::Original;
    testRuntime.executeGraph();
  }

  // now /somewhere/message is Corrupted message
  dafg::NodeHandle hijackerHandle =
    (dafg::root() / "somewhere").registerNode("hijacker", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      registry.create("message", dafg::History::No).blob(Message::Corrupted);
    });

  {
    expectedMessage = Message::Corrupted;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Hijacker disappears", "[name resolution]")
{
  TestRuntime testRuntime{};

  Message expectedMessage;

  dafg::NodeHandle senderHandle = dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("message", dafg::History::No).blob(Message::Original);
  });

  dafg::NodeHandle receiverHandle =
    (dafg::root() / "somewhere").registerNode("receiver", DAFG_PP_NODE_SRC, [&expectedMessage](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto messageHandle = registry.read("message").blob<Message>().handle();
      return [&expectedMessage, messageHandle] { CHECK(*messageHandle.get() == expectedMessage); };
    });

  // /somewhere/Message is Corrupted message
  bool isHijackerAlive = true;
  dafg::NodeHandle hijackerHandle =
    (dafg::root() / "somewhere").registerNode("hijacker", DAFG_PP_NODE_SRC, [&isHijackerAlive](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      registry.create("message", dafg::History::No).blob(Message::Corrupted);
      return [&isHijackerAlive] { CHECK(isHijackerAlive); };
    });

  {
    isHijackerAlive = true;
    expectedMessage = Message::Corrupted;
    testRuntime.executeGraph();
  }

  hijackerHandle = {};

  {
    isHijackerAlive = false;
    expectedMessage = Message::Original;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Sender became useless", "[prunning]")
{
  TestRuntime testRuntime{};

  bool isSenderPruned = false;
  dafg::NodeHandle senderHandle = dafg::register_node("sender", DAFG_PP_NODE_SRC, [&isSenderPruned](dafg::Registry registry) {
    registry.create("message", dafg::History::No).blob(Message::Original);
    return [&isSenderPruned] { CHECK_FALSE(isSenderPruned); };
  });

  dafg::NodeHandle receiverHandle = dafg::root().registerNode("receiver", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read("message").blob<Message>();
  });

  {
    isSenderPruned = false;
    testRuntime.executeGraph();
  }

  receiverHandle = dafg::root().registerNode("receiver", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.executionHas(dafg::SideEffects::External); });

  {
    isSenderPruned = true;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Sender became useful", "[prunning]")
{
  TestRuntime testRuntime{};

  bool isSenderPruned = true;
  dafg::NodeHandle senderHandle = dafg::register_node("sender", DAFG_PP_NODE_SRC, [&isSenderPruned](dafg::Registry registry) {
    registry.create("message", dafg::History::No).blob(Message::Original);
    return [&isSenderPruned] { CHECK_FALSE(isSenderPruned); };
  });

  dafg::NodeHandle receiverHandle = dafg::root().registerNode("receiver", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.executionHas(dafg::SideEffects::External); });

  {
    isSenderPruned = true;
    testRuntime.executeGraph();
  }

  receiverHandle = dafg::root().registerNode("receiver", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read("message").blob<Message>();
  });

  {
    isSenderPruned = false;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Unfulfilled optional texture request", "[optional requests]")
{
  TestRuntime testRuntime{};

  bool nodeExecuted = false;

  auto node = dafg::register_node("node", DAFG_PP_NODE_SRC, [&nodeExecuted](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);

    auto texA = registry.read("texA").texture().optional().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    return [&nodeExecuted, texA] {
      CHECK(texA.get() == nullptr);
      nodeExecuted = true;
    };
  });

  testRuntime.executeGraph();
  CHECK(nodeExecuted);
}

TEST_CASE("Two identical unfulfilled optional requests", "[optional requests]")
{
  // rationale: if we handle missing request name resolution incorrectly issues can occur here
  TestRuntime testRuntime{};

  bool nodeExecuted = false;

  auto node = dafg::register_node("node", DAFG_PP_NODE_SRC, [&nodeExecuted](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);

    auto texA = registry.readTexture("texA").optional().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto texB = registry.readTexture("texB").optional().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    return [&nodeExecuted, texA, texB] {
      CHECK(texA.get() == nullptr);
      CHECK(texB.get() == nullptr);
      nodeExecuted = true;
    };
  });

  testRuntime.executeGraph();
  CHECK(nodeExecuted);
}

TEST_CASE("Optional blob request stops being fulfilled", "[optional requests]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle sender = dafg::register_node("sender", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.create("message", dafg::History::No).blob<int>(); });

  bool expectBlobPresent = false;

  dafg::NodeHandle receiver = dafg::root().registerNode("receiver", DAFG_PP_NODE_SRC, [&expectBlobPresent](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto messageHandle = registry.readBlob<int>("message").optional().handle();
    return [&expectBlobPresent, messageHandle] {
      if (expectBlobPresent)
        CHECK(messageHandle.get() != nullptr);
      else
        CHECK(messageHandle.get() == nullptr);
    };
  });

  {
    expectBlobPresent = true;
    testRuntime.executeGraph();
  }

  sender = {};

  {
    expectBlobPresent = false;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Optional blob request becomes fulfilled", "[optional requests]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle sender = {};
  bool expectBlobPresent = false;

  dafg::NodeHandle receiver = dafg::root().registerNode("receiver", DAFG_PP_NODE_SRC, [&expectBlobPresent](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto messageHandle = registry.readBlob<int>("message").optional().handle();
    return [&expectBlobPresent, messageHandle] {
      if (expectBlobPresent)
        CHECK(messageHandle.get() != nullptr);
      else
        CHECK(messageHandle.get() == nullptr);
    };
  });

  {
    expectBlobPresent = false;
    testRuntime.executeGraph();
  }

  sender = dafg::register_node("sender", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.create("message", dafg::History::No).blob<int>(); });

  {
    expectBlobPresent = true;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Optional request stops being fulfilled from parent namespace but then goes back", "[optional requests][name resolution]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle sender = {};
  bool expectBlobPresent = false;

  auto somewhere = dafg::root() / "somewhere";
  dafg::NodeHandle receiver = somewhere.registerNode("receiver", DAFG_PP_NODE_SRC, [&expectBlobPresent](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto messageHandle = registry.readBlob<int>("message").optional().handle();
    return [&expectBlobPresent, messageHandle] {
      if (expectBlobPresent)
        CHECK(messageHandle.get() != nullptr);
      else
        CHECK(messageHandle.get() == nullptr);
    };
  });

  sender = dafg::register_node("sender", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.create("message", dafg::History::No).blob<int>(); });

  {
    expectBlobPresent = true;
    testRuntime.executeGraph();
  }

  sender = {};

  {
    expectBlobPresent = false;
    testRuntime.executeGraph();
  }

  sender = dafg::register_node("sender", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.create("message", dafg::History::No).blob<int>(); });

  {
    expectBlobPresent = true;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Change producer")
{
  TestRuntime testRuntime{};

  uint32_t testValue;
  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&testValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto objectHandle = registry.readBlob<uint32_t>("object").handle();
    return [&testValue, objectHandle] { CHECK(*objectHandle.get() == testValue); };
  });

  {
    auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC,
      [](dafg::Registry registry) { registry.create("object", dafg::History::No).blob(1u); });

    testValue = 1u;
    testRuntime.executeGraph();
  }

  // and now we have changed the settings and the "object" is produced by the new node
  {
    auto producerHandle = dafg::register_node("new_producer", DAFG_PP_NODE_SRC,
      [](dafg::Registry registry) { registry.create("object", dafg::History::No).blob(2u); });

    testValue = 2u;
    testRuntime.executeGraph();
  }
}

static dafg::NodeHandle create_producer(bool use_rename, uint32_t own_blob_value)
{
  return dafg::register_node("producer", DAFG_PP_NODE_SRC, [=](dafg::Registry registry) {
    if (use_rename)
      registry.rename("hidden_resource", "resource", dafg::History::No);
    else
      registry.create("resource", dafg::History::No).blob(own_blob_value);
  });
}

TEST_CASE("From rename to new resource", "[resource renaming]")
{
  TestRuntime testRuntime{};

  uint32_t expectedValue;
  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&expectedValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto resourceHandle = registry.readBlob<uint32_t>("resource").handle();
    return [resourceHandle, &expectedValue] { CHECK(*resourceHandle.get() == expectedValue); };
  });

  {
    auto producerHandle = create_producer(true, ~0u);

    auto hiddenProducerHandle = dafg::register_node("Hidden producer", DAFG_PP_NODE_SRC,
      [](dafg::Registry registry) { registry.create("hidden_resource", dafg::History::No).blob(1u); });

    expectedValue = 1u;
    testRuntime.executeGraph();
  }

  {
    auto producerHandle = create_producer(false, 2u);

    auto hiddenProducerHandle = dafg::register_node("Hidden producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.create("hidden_resource", dafg::History::No).blob(1u);
      return [] { CHECK(false); };
    });

    expectedValue = 2u;
    testRuntime.executeGraph();
  }
}

TEST_CASE("From new resource to rename", "[resource renaming]")
{
  TestRuntime testRuntime{};

  uint32_t expectedValue;
  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&expectedValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto resourceHandle = registry.readBlob<uint32_t>("resource").handle();
    return [resourceHandle, &expectedValue] { CHECK(*resourceHandle.get() == expectedValue); };
  });

  {
    auto producerHandle = create_producer(false, 2u);

    auto hiddenProducerHandle = dafg::register_node("Hidden producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.create("hidden_resource", dafg::History::No).blob(1u);
      return [] { CHECK(false); };
    });

    expectedValue = 2u;
    testRuntime.executeGraph();
  }

  {
    auto producerHandle = create_producer(true, ~0u);

    auto hiddenProducerHandle = dafg::register_node("Hidden producer", DAFG_PP_NODE_SRC,
      [](dafg::Registry registry) { registry.create("hidden_resource", dafg::History::No).blob(1u); });

    expectedValue = 1u;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Test optional shader var", "[shader vars]")
{
  TestRuntime testRuntime{};

  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.read("blob").blob<uint32_t>().optional().bindToShaderVar("blob");
  });

  // TODO: currently this test just checks that we have recompiled the graph correctly
  // and the bindings are contained in the node's resourceRequest.
  // In the future it will be useful to add a real bindump and check that the real values are set correctly

  dafg::NodeHandle producerHandle;
  for (uint32_t i = 0; i < 5; ++i)
  {
    if (i % 2)
    {
      producerHandle = {};
    }
    else
    {
      producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC,
        [](dafg::Registry registry) { registry.create("blob", dafg::History::No).blob(1u); });
    }
    testRuntime.executeGraph();
  }
}

TEST_CASE("Simple history request", "[history requests]")
{
  TestRuntime testRuntime{};

  uint32_t newValue = 1;
  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [&newValue](dafg::Registry registry) {
    auto blobHandle = registry.create("blob", dafg::History::ClearZeroOnFirstFrame).blob<uint32_t>({}).handle();
    return [blobHandle, &newValue] { blobHandle.ref() = newValue; };
  });

  uint32_t expectedHistory = 0; // we expect default value for first frame
  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&newValue, &expectedHistory](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto blobHandle = registry.readBlob<uint32_t>("blob").handle();
    auto blobHistoryHandle = registry.readBlobHistory<uint32_t>("blob").handle();

    return [blobHandle, blobHistoryHandle, &newValue, &expectedHistory] {
      CHECK(*blobHandle.get() == newValue);
      CHECK(*blobHistoryHandle.get() == expectedHistory);
    };
  });

  {
    expectedHistory = 0;
    newValue = 1;
    testRuntime.executeGraph();
  }

  {
    expectedHistory = eastl::exchange(newValue, 2);
    testRuntime.executeGraph();
  }
}

TEST_CASE("Resolve history request when hijacker appears", "[history requests][name resolution]")
{
  TestRuntime testRuntime{};

  uint32_t newOriginalValue = 1;
  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [&newOriginalValue](dafg::Registry registry) {
    auto blobHandle = registry.create("blob", dafg::History::ClearZeroOnFirstFrame).blob<uint32_t>({}).handle();
    return [blobHandle, &newOriginalValue] { blobHandle.ref() = newOriginalValue; };
  });

  uint32_t expectValue = 1;
  uint32_t expectedHistory = 0; // we expect default value for first frame
  auto consumerHandle =
    (dafg::root() / "somewhere").registerNode("consumer", DAFG_PP_NODE_SRC, [&expectValue, &expectedHistory](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto blobHandle = registry.readBlob<uint32_t>("blob").handle();
      auto blobHistoryHandle = registry.readBlobHistory<uint32_t>("blob").handle();

      return [blobHandle, blobHistoryHandle, &expectValue, &expectedHistory] {
        CHECK(*blobHandle.get() == expectValue);
        CHECK(*blobHistoryHandle.get() == expectedHistory);
      };
    });

  {
    expectedHistory = 0;
    expectValue = newOriginalValue = 1;
    testRuntime.executeGraph();
  }

  uint32_t newHijackedValue = 2;
  dafg::NodeHandle hijackerHandle =
    (dafg::root() / "somewhere").registerNode("hijacker", DAFG_PP_NODE_SRC, [&newHijackedValue](dafg::Registry registry) {
      auto blobHandle = registry.create("blob", dafg::History::ClearZeroOnFirstFrame).blob<uint32_t>({}).handle();
      return [blobHandle, &newHijackedValue] { blobHandle.ref() = newHijackedValue; };
    });

  {
    newOriginalValue = ~0;
    expectedHistory = 0;
    expectValue = newHijackedValue = 2;
    testRuntime.executeGraph();
  }

  {
    newOriginalValue = ~0;
    expectedHistory = eastl::exchange(newHijackedValue, 4);
    expectValue = newHijackedValue;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Resolve history request when hijacker disappears", "[history requests][name resolution]")
{
  TestRuntime testRuntime{};

  uint32_t newOriginalValue = 1;
  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [&newOriginalValue](dafg::Registry registry) {
    auto blobHandle = registry.create("blob", dafg::History::ClearZeroOnFirstFrame).blob<uint32_t>({}).handle();
    return [blobHandle, &newOriginalValue] { blobHandle.ref() = newOriginalValue; };
  });

  uint32_t expectValue = 1;
  uint32_t expectedHistory = 0; // we expect default value for first frame
  auto consumerHandle =
    (dafg::root() / "somewhere").registerNode("consumer", DAFG_PP_NODE_SRC, [&expectValue, &expectedHistory](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto blobHandle = registry.readBlob<uint32_t>("blob").handle();
      auto blobHistoryHandle = registry.readBlobHistory<uint32_t>("blob").handle();

      return [blobHandle, blobHistoryHandle, &expectValue, &expectedHistory] {
        CHECK(*blobHandle.get() == expectValue);
        CHECK(*blobHistoryHandle.get() == expectedHistory);
      };
    });

  uint32_t newHijackedValue = 2;
  dafg::NodeHandle hijackerHandle =
    (dafg::root() / "somewhere").registerNode("hijacker", DAFG_PP_NODE_SRC, [&newHijackedValue](dafg::Registry registry) {
      auto blobHandle = registry.create("blob", dafg::History::ClearZeroOnFirstFrame).blob<uint32_t>({}).handle();
      return [blobHandle, &newHijackedValue] { blobHandle.ref() = newHijackedValue; };
    });

  {
    expectedHistory = 0;
    newOriginalValue = ~0;
    expectValue = newHijackedValue = 2;
    testRuntime.executeGraph();
  }

  hijackerHandle = {};

  {
    expectValue = newOriginalValue = 1;
    newHijackedValue = ~0;
    expectedHistory = 0; // producer was pruned, so we expect a cleared history
    testRuntime.executeGraph();
  }

  {
    newHijackedValue = ~0;
    expectedHistory = eastl::exchange(newOriginalValue, 3);
    expectValue = newOriginalValue;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Simple history request for rename", "[history requests][resource renaming]")
{
  TestRuntime testRuntime{};

  uint32_t newValue = 1;
  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [&newValue](dafg::Registry registry) {
    auto blobHandle = registry.create("blob", dafg::History::No).blob<uint32_t>({}).handle();
    return [blobHandle, &newValue] { blobHandle.ref() = newValue; };
  });

  auto renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameBlob<uint32_t>("blob", "renamed_blob", dafg::History::ClearZeroOnFirstFrame); });

  uint32_t expectedHistory = 0; // we expect default value for first frame
  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&newValue, &expectedHistory](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto renamedBlobHandle = registry.readBlob<uint32_t>("renamed_blob").handle();
    auto renamedBlobHistoryHandle = registry.readBlobHistory<uint32_t>("renamed_blob").handle();

    return [renamedBlobHandle, renamedBlobHistoryHandle, &newValue, &expectedHistory] {
      CHECK(*renamedBlobHandle.get() == newValue);
      CHECK(*renamedBlobHistoryHandle.get() == expectedHistory);
    };
  });

  {
    expectedHistory = 0;
    newValue = 1;
    testRuntime.executeGraph();
  }

  {
    expectedHistory = eastl::exchange(newValue, 2);
    testRuntime.executeGraph();
  }
}

TEST_CASE("History request for modified rename", "[history requests][resource renaming]")
{
  TestRuntime testRuntime{};

  uint32_t newValue = 1;
  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [&newValue](dafg::Registry registry) {
    auto blobHandle = registry.create("blob", dafg::History::No).blob<uint32_t>({}).handle();
    return [blobHandle, &newValue] { blobHandle.ref() = newValue; };
  });

  uint32_t newRenamedValue = 2;
  auto renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [&newRenamedValue](dafg::Registry registry) {
    auto renamedBlobHandle = registry.renameBlob<uint32_t>("blob", "renamed_blob", dafg::History::ClearZeroOnFirstFrame).handle();
    return [renamedBlobHandle, &newRenamedValue] { renamedBlobHandle.ref() = newRenamedValue; };
  });

  uint32_t expectedHistory = 0; // we expect default value for first frame
  auto consumerHandle =
    dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&newRenamedValue, &expectedHistory](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto renamedBlobHandle = registry.readBlob<uint32_t>("renamed_blob").handle();
      auto renamedBlobHistoryHandle = registry.readBlobHistory<uint32_t>("renamed_blob").handle();

      return [renamedBlobHandle, renamedBlobHistoryHandle, &newRenamedValue, &expectedHistory] {
        CHECK(*renamedBlobHandle.get() == newRenamedValue);
        CHECK(*renamedBlobHistoryHandle.get() == expectedHistory);
      };
    });

  {
    expectedHistory = 0;
    newValue = 1;
    newRenamedValue = 2;
    testRuntime.executeGraph();
  }

  {
    newValue = 3;
    expectedHistory = eastl::exchange(newRenamedValue, 4);
    testRuntime.executeGraph();
  }
}


TEST_CASE("Multiple optional requests flip between fulfilled and not", "[optional requests][name resolver]")
{
  TestRuntime testRuntime{};

  bool expectOptionalBlob = true;
  bool expectOptionalTexture = true;
  dafg::NodeHandle consumerHandle =
    dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&expectOptionalBlob, &expectOptionalTexture](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto blobHandle = registry.read("optional_blob").blob<uint32_t>().optional().handle();
      auto texHandle = registry.readTexture("optional_texture").atStage(dafg::Stage::PS).bindToShaderVar("shvar").optional().handle();

      return [blobHandle, texHandle, &expectOptionalBlob, &expectOptionalTexture]() {
        if (expectOptionalBlob)
          CHECK_FALSE(blobHandle.get() == nullptr);
        else
          CHECK(blobHandle.get() == nullptr);

        if (expectOptionalTexture)
          CHECK_FALSE(texHandle.get() == nullptr);
        else
          CHECK(texHandle.get() == nullptr);
      };
    });

  auto createBlobProducer = [](bool create_node) -> dafg::NodeHandle {
    if (create_node)
      return dafg::register_node("blob_producer", DAFG_PP_NODE_SRC,
        [](dafg::Registry registry) { registry.create("optional_blob", dafg::History::No).blob<uint32_t>(); });
    return {};
  };

  auto createTexProducer = [](bool create_node) -> dafg::NodeHandle {
    if (create_node)
      return dafg::register_node("tex_producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        registry.create("optional_texture", dafg::History::No)
          .texture(dafg::Texture2dCreateInfo{.creationFlags = TEXFMT_R8, .resolution = IPoint2{1, 1}});
      });
    return {};
  };

  // our incremental name resolver must pass all cycle
  for (uint32_t i = 0; i < 5; ++i)
  {
    expectOptionalBlob = i & 1;
    expectOptionalTexture = (i >> 1) & 1;
    auto blobProducerHandle = createBlobProducer(expectOptionalBlob);
    auto texProducerHandle = createTexProducer(expectOptionalTexture);
    testRuntime.executeGraph();
  }
}

TEST_CASE("Change resource in slot", "[resource slots][name resolver]")
{
  TestRuntime testRuntime{};

  auto somewhereNS = dafg::root() / "somewhere";

  Message expectedMessage = Message::Original;
  dafg::NodeHandle receiverHandle =
    somewhereNS.registerNode("receiver", DAFG_PP_NODE_SRC, [&expectedMessage](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto messageHandle = registry.readBlob<Message>("received_message").handle();
      return [messageHandle, &expectedMessage] { CHECK(messageHandle.ref() == expectedMessage); };
    });

  dafg::NodeHandle senderHandle = dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("original_message", dafg::History::No).blob<Message>(Message::Original);
    registry.create("corrupted_message", dafg::History::No).blob<Message>(Message::Corrupted);
  });

  {
    somewhereNS.fillSlot(dafg::NamedSlot{"received_message"}, dafg::root(), "original_message");
    expectedMessage = Message::Original;
    testRuntime.executeGraph();
  }

  {
    somewhereNS.fillSlot(dafg::NamedSlot{"received_message"}, dafg::root(), "corrupted_message");
    expectedMessage = Message::Corrupted;
    testRuntime.executeGraph();
  }
}

TEST_CASE("Optional request gets fulfilled by a slot being filled", "[resource slots][name resolver][optional requests]")
{
  TestRuntime testRuntime{};

  auto somewhereNS = dafg::root() / "somewhere";

  bool expectOptionalBlob = false;
  uint32_t expectedValue = 1;
  dafg::NodeHandle receiverHandle =
    somewhereNS.registerNode("receiver", DAFG_PP_NODE_SRC, [&expectOptionalBlob, &expectedValue](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto blobHandle = registry.readBlob<uint32_t>("received_blob").optional().handle();
      return [blobHandle, &expectOptionalBlob, &expectedValue] {
        if (expectOptionalBlob)
          CHECK(*blobHandle.get() == expectedValue);
        else
          CHECK(blobHandle.get() == nullptr);
      };
    });

  dafg::NodeHandle senderHandle = dafg::register_node("sender", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.create("blob", dafg::History::No).blob(1u); });

  {
    expectedValue = -1;
    expectOptionalBlob = false;
    testRuntime.executeGraph();
  }

  {
    somewhereNS.fillSlot(dafg::NamedSlot{"received_blob"}, dafg::root(), "blob");
    expectedValue = 1;
    expectOptionalBlob = true;
    testRuntime.executeGraph();
  }
}
