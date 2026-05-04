// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "testRuntime.h"
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_test_macros.hpp>
#include <math/random/dag_random.h>
#include <drv/3d/dag_interface_table.h>
#include <memory/dag_framemem.h>
#include <EASTL/vector.h>


enum class Message
{
  Original,
  Corrupted,
};

TEST_CASE("Empty graph", "")
{
  TestRuntime testRuntime{};
  testRuntime.executeGraph();
}

TEST_CASE("Hijacker appears", "[name resolution]")
{
  TestRuntime testRuntime{};

  Message expectedMessage;

  // /somewhere/message is Original message
  dafg::NodeHandle senderHandle = dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("message").blob(Message::Original);
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
      registry.create("message").blob(Message::Corrupted);
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
    registry.create("message").blob(Message::Original);
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
      registry.create("message").blob(Message::Corrupted);
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
    registry.create("message").blob(Message::Original);
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
    registry.create("message").blob(Message::Original);
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

  dafg::NodeHandle sender =
    dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("message").blob<int>(); });

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

  sender = dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("message").blob<int>(); });

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

  sender = dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("message").blob<int>(); });

  {
    expectBlobPresent = true;
    testRuntime.executeGraph();
  }

  sender = {};

  {
    expectBlobPresent = false;
    testRuntime.executeGraph();
  }

  sender = dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("message").blob<int>(); });

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
    auto producerHandle =
      dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("object").blob(1u); });

    testValue = 1u;
    testRuntime.executeGraph();
  }

  // and now we have changed the settings and the "object" is produced by the new node
  {
    auto producerHandle =
      dafg::register_node("new_producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("object").blob(2u); });

    testValue = 2u;
    testRuntime.executeGraph();
  }
}

static dafg::NodeHandle create_producer(bool use_rename, uint32_t own_blob_value)
{
  return dafg::register_node("producer", DAFG_PP_NODE_SRC, [=](dafg::Registry registry) {
    if (use_rename)
      registry.rename("hidden_resource", "resource");
    else
      registry.create("resource").blob(own_blob_value);
  });
}

TEST_CASE("Simple rename", "[resource renaming]")
{
  TestRuntime testRuntime{};

  auto producerHndl = dafg::register_node("producer", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.create("original_resource").blob<uint32_t>(1u); });

  auto renamerHndl = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [&](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.rename("original_resource", "renamed_resource");
  });

  testRuntime.executeGraph();
  testRuntime.executeGraph();
}

TEST_CASE("New renamer appears", "[resource renaming]")
{
  TestRuntime testRuntime{};

  enum class Step
  {
    NotRun,
    Create,
    Rename1,
    Rename2,
  };

  Step currentStep = Step::NotRun;

  auto producerHndl = dafg::register_node("producer", DAFG_PP_NODE_SRC, [&](dafg::Registry registry) {
    registry.create("original_resource").blob<uint32_t>(1u);
    return [&] { CHECK(eastl::exchange(currentStep, Step::Create) == Step::NotRun); };
  });

  auto renamerHndl = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [&](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.rename("original_resource", "renamed_resource");
    return [&] { CHECK(eastl::exchange(currentStep, Step::Rename1) == Step::Create); };
  });

  testRuntime.executeGraph();

  CHECK(currentStep == Step::Rename1);
  currentStep = Step::NotRun;

  auto secondRenamerHndl = dafg::register_node("second_renamer", DAFG_PP_NODE_SRC, [&](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.rename("renamed_resource", "final_resource");
    return [&] { CHECK(eastl::exchange(currentStep, Step::Rename2) == Step::Rename1); };
  });

  testRuntime.executeGraph();

  CHECK(currentStep == Step::Rename2);
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
      [](dafg::Registry registry) { registry.create("hidden_resource").blob(1u); });

    expectedValue = 1u;
    testRuntime.executeGraph();
  }

  {
    auto producerHandle = create_producer(false, 2u);

    auto hiddenProducerHandle = dafg::register_node("Hidden producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.create("hidden_resource").blob(1u);
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
      registry.create("hidden_resource").blob(1u);
      return [] { CHECK(false); };
    });

    expectedValue = 2u;
    testRuntime.executeGraph();
  }

  {
    auto producerHandle = create_producer(true, ~0u);

    auto hiddenProducerHandle = dafg::register_node("Hidden producer", DAFG_PP_NODE_SRC,
      [](dafg::Registry registry) { registry.create("hidden_resource").blob(1u); });

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
      producerHandle =
        dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("blob").blob(1u); });
    }
    testRuntime.executeGraph();
  }
}

TEST_CASE("Simple history request", "[history requests]")
{
  TestRuntime testRuntime{};

  uint32_t newValue = 1;
  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [&newValue](dafg::Registry registry) {
    auto blobHandle = registry.create("blob").blob<uint32_t>({}).withHistory().handle();
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
    auto blobHandle = registry.create("blob").blob<uint32_t>({}).withHistory().handle();
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
      auto blobHandle = registry.create("blob").blob<uint32_t>({}).withHistory().handle();
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
    auto blobHandle = registry.create("blob").blob<uint32_t>({}).withHistory().handle();
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
      auto blobHandle = registry.create("blob").blob<uint32_t>({}).withHistory().handle();
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
    auto blobHandle = registry.create("blob").blob<uint32_t>({}).handle();
    return [blobHandle, &newValue] { blobHandle.ref() = newValue; };
  });

  auto renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameBlob<uint32_t>("blob", "renamed_blob").withHistory(); });

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
    auto blobHandle = registry.create("blob").blob<uint32_t>({}).handle();
    return [blobHandle, &newValue] { blobHandle.ref() = newValue; };
  });

  uint32_t newRenamedValue = 2;
  auto renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [&newRenamedValue](dafg::Registry registry) {
    auto renamedBlobHandle = registry.renameBlob<uint32_t>("blob", "renamed_blob").withHistory().handle();
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

TEST_CASE("Rename disappears & history request changes to original", "[history requests][resource renaming]")
{
  TestRuntime testRuntime{};

  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto blobHandle = registry.create("blob").blob<uint32_t>().handle();
    return [blobHandle] { blobHandle.ref() = 1; };
  });

  auto renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto renamedBlobHandle = registry.renameBlob<uint32_t>("blob", "renamed_blob").withHistory().handle();
    return [renamedBlobHandle] { renamedBlobHandle.ref() = 2; };
  });

  uint32_t expectedValue = 0;
  uint32_t expectedHistory = 0;
  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&expectedHistory, &expectedValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto renamedBlobHandle = registry.readBlob<uint32_t>("renamed_blob").handle();
    auto renamedBlobHistoryHandle = registry.readBlobHistory<uint32_t>("renamed_blob").handle();

    return [renamedBlobHandle, renamedBlobHistoryHandle, &expectedHistory, &expectedValue] {
      CHECK(*renamedBlobHandle.get() == expectedValue);
      CHECK(*renamedBlobHistoryHandle.get() == expectedHistory);
    };
  });

  {
    expectedValue = 2;
    expectedHistory = 0;
    testRuntime.executeGraph();
  }
  {
    expectedValue = 2;
    expectedHistory = 2;
    testRuntime.executeGraph();
  }

  producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto blobHandle = registry.create("blob").blob<uint32_t>().withHistory().handle();
    return [blobHandle] { blobHandle.ref() = 1; };
  });

  renamerHandle = {};

  consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&expectedHistory, &expectedValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto renamedBlobHandle = registry.readBlob<uint32_t>("blob").handle();
    auto renamedBlobHistoryHandle = registry.readBlobHistory<uint32_t>("blob").handle();

    return [renamedBlobHandle, renamedBlobHistoryHandle, &expectedHistory, &expectedValue] {
      CHECK(*renamedBlobHandle.get() == expectedValue);
      CHECK(*renamedBlobHistoryHandle.get() == expectedHistory);
    };
  });

  {
    expectedValue = 1;
    expectedHistory = 0;
    testRuntime.executeGraph();
  }
  {
    expectedValue = 1;
    expectedHistory = 1;
    testRuntime.executeGraph();
  }
}

TEST_CASE("History request & flag disappear without lifetime changing", "[history requests][resource renaming]")
{
  TestRuntime testRuntime{};

  auto producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto blobHandle = registry.create("blob").blob<uint32_t>().handle();
    return [blobHandle] { blobHandle.ref() = 1; };
  });

  auto renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto renamedBlobHandle = registry.renameBlob<uint32_t>("blob", "renamed_blob").withHistory().handle();
    return [renamedBlobHandle] { renamedBlobHandle.ref() = 2; };
  });

  uint32_t expectedValue = 0;
  uint32_t expectedHistory = 0;
  auto consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&expectedHistory, &expectedValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto renamedBlobHandle = registry.readBlob<uint32_t>("renamed_blob").handle();
    auto renamedBlobHistoryHandle = registry.readBlobHistory<uint32_t>("renamed_blob").handle();

    return [renamedBlobHandle, renamedBlobHistoryHandle, &expectedHistory, &expectedValue] {
      CHECK(*renamedBlobHandle.get() == expectedValue);
      CHECK(*renamedBlobHistoryHandle.get() == expectedHistory);
    };
  });

  {
    expectedValue = 2;
    expectedHistory = 0;
    testRuntime.executeGraph();
  }
  {
    expectedValue = 2;
    expectedHistory = 2;
    testRuntime.executeGraph();
  }

  renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto renamedBlobHandle = registry.renameBlob<uint32_t>("blob", "renamed_blob").handle();
    return [renamedBlobHandle] { renamedBlobHandle.ref() = 2; };
  });

  consumerHandle = {};

  consumerHandle = dafg::register_node("consumer", DAFG_PP_NODE_SRC, [&expectedValue](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto renamedBlobHandle = registry.readBlob<uint32_t>("renamed_blob").handle();

    return [renamedBlobHandle, &expectedValue] { CHECK(*renamedBlobHandle.get() == expectedValue); };
  });

  {
    expectedValue = 2;
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
        [](dafg::Registry registry) { registry.create("optional_blob").blob<uint32_t>(); });
    return {};
  };

  auto createTexProducer = [](bool create_node) -> dafg::NodeHandle {
    if (create_node)
      return dafg::register_node("tex_producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        registry.create("optional_texture")
          .texture(dafg::Texture2dCreateInfo{.creationFlags = TEXFMT_R8, .resolution = IPoint2{1, 1}})
          .atStage(dafg::Stage::PS)
          .useAs(dafg::Usage::SHADER_RESOURCE);
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
    registry.create("original_message").blob<Message>(Message::Original);
    registry.create("corrupted_message").blob<Message>(Message::Corrupted);
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

  dafg::NodeHandle senderHandle =
    dafg::register_node("sender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { registry.create("blob").blob(1u); });

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

TEST_CASE("Get optional renamed resource", "[resource renaming][optional requests]")
{
  TestRuntime testRuntime{};

  bool expectOptionalBlob = false;
  uint32_t expectedValue = 1;
  dafg::NodeHandle renamerHandle =
    dafg::register_node("renamer", DAFG_PP_NODE_SRC, [&expectOptionalBlob, &expectedValue](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);

      auto renamedBlobHandle = registry.renameBlob<uint32_t>("original_blob", "renamed_blob").optional().handle();

      return [renamedBlobHandle, &expectOptionalBlob, &expectedValue] {
        if (expectOptionalBlob)
        {
          REQUIRE_FALSE(renamedBlobHandle.get() == nullptr);
          CHECK(*renamedBlobHandle.get() == expectedValue);
        }
        else
          CHECK(renamedBlobHandle.get() == nullptr);
      };
    });

  {
    expectOptionalBlob = false;
    expectedValue = ~0u;
    testRuntime.executeGraph();
  }

  dafg::NodeHandle producerHandle = dafg::register_node("producer", DAFG_PP_NODE_SRC,
    [&expectedValue](dafg::Registry registry) { registry.create("original_blob").blob(1u); });

  {
    expectOptionalBlob = true;
    expectedValue = 1;
    testRuntime.executeGraph();
  }

  producerHandle = {};
  {
    expectOptionalBlob = false;
    expectedValue = ~0u;
    testRuntime.executeGraph();
  }
}


TEST_CASE("Auto-resolution from root to local namespace", "[name resolution]")
{
  TestRuntime testRuntime{};

  IPoint2 expectedResolution;

  dafg::root().setResolution("resolution", IPoint2{4, 4});

  dafg::NodeHandle providerHandle =
    (dafg::root() / "somewhere").registerNode("texture_provider", DAFG_PP_NODE_SRC, [&expectedResolution](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      auto texHandle = registry.create("texture")
                         .texture({.creationFlags = TEXFMT_R8, .resolution = registry.getResolution<2>("resolution")})
                         .atStage(dafg::Stage::PS)
                         .useAs(dafg::Usage::COLOR_ATTACHMENT)
                         .handle();
      return [texHandle, &expectedResolution]() {
        TextureInfo textureInfo;
        texHandle.get()->getinfo(textureInfo);
        CHECK((IPoint2{textureInfo.w, textureInfo.h} == expectedResolution));
      };
    });

  {
    expectedResolution = IPoint2{4, 4};
    testRuntime.executeGraph();
  }

  {
    (dafg::root() / "somewhere").setResolution("resolution", IPoint2{8, 8});

    expectedResolution = IPoint2{8, 8};
    testRuntime.executeGraph();
  }
}


TEST_CASE("Runtime deduces TEXCF_RTARGET from usage in creator node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle rendererHandle = dafg::register_node("renderer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle = registry.create("created_texture")
                       .texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}})
                       .atStage(dafg::Stage::PS)
                       .useAs(dafg::Usage::COLOR_ATTACHMENT)
                       .handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_RTARGET));
    };
  });

  testRuntime.executeGraph();
}

TEST_CASE("Runtime deduces TEXCF_UPDATE_DESTINATION from usage in creator node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle rendererHandle = dafg::register_node("renderer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle = registry.create("created_texture")
                       .texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}})
                       .atStage(dafg::Stage::TRANSFER)
                       .useAs(dafg::Usage::COPY)
                       .handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_UPDATE_DESTINATION));
    };
  });

  testRuntime.executeGraph();
}

TEST_CASE("Runtime deduces TEXCF_RTARGET from usage in modifier node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("provided_texture").texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}});
  });

  dafg::NodeHandle rendererHandle = dafg::register_node("renderer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle = registry.modifyTexture("provided_texture").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_RTARGET));
    };
  });

  testRuntime.executeGraph();
}

TEST_CASE("Runtime deduces TEXCF_UPDATE_DESTINATION from usage in modifier node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("provided_texture").texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}});
  });

  dafg::NodeHandle rendererHandle = dafg::register_node("renderer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle = registry.modifyTexture("provided_texture").atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY).handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_UPDATE_DESTINATION));
    };
  });

  testRuntime.executeGraph();
}

TEST_CASE("Runtime deduces TEXCF_VARIABLE_RATE from usage in reader node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("provided_texture")
      .texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}})
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);
  });

  dafg::NodeHandle rendererHandle = dafg::register_node("renderer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle = registry.readTexture("provided_texture").atStage(dafg::Stage::PS).useAs(dafg::Usage::VRS_RATE_TEXTURE).handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_VARIABLE_RATE));
    };
  });

  testRuntime.executeGraph();
}


TEST_CASE("Runtime deduces TEXCF_UNORDERED from usage in creator node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle = registry.create("created_texture")
                       .texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}})
                       .atStage(dafg::Stage::COMPUTE)
                       .useAs(dafg::Usage::SHADER_RESOURCE)
                       .handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_UNORDERED));
    };
  });

  testRuntime.executeGraph();
}

TEST_CASE("Runtime deduces TEXCF_UNORDERED from usage in modifier node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("provided_texture").texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}});
  });

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle =
      registry.modifyTexture("provided_texture").atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_UNORDERED));
    };
  });

  testRuntime.executeGraph();
}


TEST_CASE("Runtime deduces SBCF_BIND_UNORDERED from usage in creator node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto bufHandle = registry.create("created_buffer")
                       .buffer({4, 3, 0, 0})
                       .atStage(dafg::Stage::COMPUTE)
                       .useAs(dafg::Usage::SHADER_RESOURCE)
                       .handle();
    return [bufHandle]() { CHECK((bufHandle.get()->getFlags() & SBCF_BIND_UNORDERED)); };
  });

  testRuntime.executeGraph();
}

TEST_CASE("Runtime deduces SBCF_BIND_UNORDERED/SBCF_BIND_SHADER_RES from usage in modifier node", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("provided_buf_to_write").buffer({4, 3, 0, 0});
    registry.create("provided_buf_to_read").buffer({4, 5, 0, 0});
  });

  dafg::NodeHandle modifierHandle = dafg::register_node("modifier", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.modify("provided_buf_to_read").buffer().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE);
  });

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto writeBufHandle =
      registry.modify("provided_buf_to_write").buffer().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto readBufHandle =
      registry.read("provided_buf_to_read").buffer().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    return [writeBufHandle, readBufHandle]() {
      CHECK((writeBufHandle.get()->getFlags() & SBCF_BIND_UNORDERED));
      CHECK((readBufHandle.get()->getFlags() & SBCF_BIND_SHADER_RES));
    };
  });

  testRuntime.executeGraph();
}


TEST_CASE("Runtime deduces SBCF_BIND_VERTEX/SBCF_BIND_INDEX/SBCF_MISC_DRAWINDIRECT/SBCF_BIND_CONSTANT from usage", "[resource flags]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("vertex_buffer").buffer({4, 3, 0, 0}).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.create("index_buffer").buffer({4, 5, 0, 0}).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.create("indirect_buffer").buffer({4, 7, 0, 0}).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE);
    registry.create("constant_buffer").buffer({4, 9, 0, 0}).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE);
  });

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto vtxBufHandle = registry.read("vertex_buffer").buffer().atStage(dafg::Stage::VS).useAs(dafg::Usage::VERTEX_BUFFER).handle();
    auto idxBufHandle = registry.read("index_buffer").buffer().atStage(dafg::Stage::VS).useAs(dafg::Usage::INDEX_BUFFER).handle();
    auto indirBufHandle =
      registry.read("indirect_buffer").buffer().atStage(dafg::Stage::VS).useAs(dafg::Usage::INDIRECTION_BUFFER).handle();
    auto constBufHandle =
      registry.read("constant_buffer").buffer().atStage(dafg::Stage::VS).useAs(dafg::Usage::CONSTANT_BUFFER).handle();
    return [vtxBufHandle, idxBufHandle, indirBufHandle, constBufHandle]() {
      CHECK((vtxBufHandle.get()->getFlags() & SBCF_BIND_VERTEX));
      CHECK((idxBufHandle.get()->getFlags() & SBCF_BIND_INDEX));
      CHECK((indirBufHandle.get()->getFlags() & SBCF_MISC_DRAWINDIRECT));
      CHECK((constBufHandle.get()->getFlags() & SBCF_BIND_CONSTANT));
    };
  });

  testRuntime.executeGraph();
}


TEST_CASE("Runtime deduces TEXCF_RTARGET from usage before rename", "[resource flags][resource renaming]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("provided_texture").texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}});
  });

  dafg::NodeHandle rendererHandle = dafg::register_node("renderer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.modify("provided_texture").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  });

  dafg::NodeHandle renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.rename("provided_texture", "renamed_texture");
  });

  dafg::NodeHandle readerHandle = dafg::register_node("reader", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle =
      registry.read("renamed_texture").texture().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_RTARGET));
    };
  });

  testRuntime.executeGraph();
}

TEST_CASE("Runtime deduces TEXCF_RTARGET from usage after rename", "[resource flags][resource renaming]")
{
  TestRuntime testRuntime{};

  dafg::NodeHandle providerHandle = dafg::register_node("provider", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("provided_texture").texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}});
  });

  dafg::NodeHandle modifierHandle = dafg::register_node("modifier", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.modify("provided_texture").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE);
  });

  dafg::NodeHandle renamerHandle = dafg::register_node("renamer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.rename("provided_texture", "renamed_texture");
  });

  dafg::NodeHandle rendererHandle = dafg::register_node("renderer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.modify("renamed_texture").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
  });

  dafg::NodeHandle readerHandle = dafg::register_node("reader", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto texHandle =
      registry.read("provided_texture").texture().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    return [texHandle]() {
      TextureInfo textureInfo;
      texHandle.get()->getinfo(textureInfo);
      CHECK((textureInfo.cflg & TEXCF_RTARGET));
    };
  });

  testRuntime.executeGraph();
}


TEST_CASE("Runtime deduces REWRITE_AS_COPY_DESTINATION/DISCARD_AS_RTV_DSV/DISCARD_AS_UAV from usage", "[resource activation]")
{
  const auto [stage, usage, action] = GENERATE(table<dafg::Stage, dafg::Usage, ResourceActivationAction>(
    {{dafg::Stage::TRANSFER, dafg::Usage::COPY, ResourceActivationAction::REWRITE_AS_COPY_DESTINATION},
      {dafg::Stage::PS, dafg::Usage::COLOR_ATTACHMENT, ResourceActivationAction::DISCARD_AS_RTV_DSV},
      {dafg::Stage::CS, dafg::Usage::SHADER_RESOURCE, ResourceActivationAction::DISCARD_AS_UAV}}));

  static ResourceActivationAction globalExpectedAction;
  D3dInterfaceTable interfaceTableCopy = d3di;

  TestRuntime testRuntime{};

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [stage, usage](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("texture").texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}}).atStage(stage).useAs(usage);
  });

  globalExpectedAction = action;
  d3di.activate_texture = [](BaseTexture *, ResourceActivationAction action, const ResourceClearValue &, GpuPipeline) -> void {
    CHECK(action == globalExpectedAction);
  };

  testRuntime.executeGraph();

  d3di = interfaceTableCopy;
}

TEST_CASE("Runtime deduces CLEAR_AS_RTV_DSV/CLEAR_F_AS_UAV/CLEAR_I_AS_UAV from usage", "[resource activation]")
{
  const auto [stage, usage, action, format] = GENERATE(table<dafg::Stage, dafg::Usage, ResourceActivationAction, uint32_t>(
    {{dafg::Stage::PS, dafg::Usage::COLOR_ATTACHMENT, ResourceActivationAction::CLEAR_AS_RTV_DSV, TEXFMT_R8G8B8A8},
      {dafg::Stage::CS, dafg::Usage::SHADER_RESOURCE, ResourceActivationAction::CLEAR_F_AS_UAV, TEXFMT_R8G8B8A8},
      {dafg::Stage::CS, dafg::Usage::SHADER_RESOURCE, ResourceActivationAction::CLEAR_I_AS_UAV, TEXFMT_R32UI}}));

  static ResourceActivationAction globalExpectedAction;
  D3dInterfaceTable interfaceTableCopy = d3di;

  TestRuntime testRuntime{};

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [stage, usage, format](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("texture")
      .texture({.creationFlags = format, .resolution = IPoint2{4, 4}})
      .atStage(stage)
      .useAs(usage)
      .clear(ResourceClearValue{});
  });

  globalExpectedAction = action;
  d3di.activate_texture = [](BaseTexture *, ResourceActivationAction action, const ResourceClearValue &, GpuPipeline) -> void {
    CHECK(action == globalExpectedAction);
  };

  testRuntime.executeGraph();

  d3di = interfaceTableCopy;
}


TEST_CASE("Runtime deduces RP_TA_LOAD_CLEAR for render pass with node with clear action", "[resource activation]")
{
  D3dInterfaceTable interfaceTableCopy = d3di;

  TestRuntime testRuntime{};

  dafg::NodeHandle userHandle = dafg::register_node("user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.create("color_tex").texture({.creationFlags = TEXFMT_R8G8B8A8, .resolution = IPoint2{4, 4}}).clear(ResourceClearValue{});
    registry.requestRenderPass().color({"color_tex"});
  });

  d3di.activate_texture = [](BaseTexture *, ResourceActivationAction action, const ResourceClearValue &, GpuPipeline) -> void {
    CHECK(action == ResourceActivationAction::REWRITE_AS_RTV_DSV);
  };

  d3di.create_render_pass = [](const RenderPassDesc &rp_desc) -> d3d::RenderPass * {
    CHECK(rp_desc.binds->action & RP_TA_LOAD_CLEAR);
    return nullptr;
  };

  testRuntime.executeGraph();

  d3di = interfaceTableCopy;
}

TEST_CASE("Declaration callback runs inside FRAMEMEM_REGION", "[framemem]")
{
  TestRuntime testRuntime{};

  constexpr int N = 128;

  auto nodeHandle = dafg::register_node("framemem_user", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    // Simulate a node that uses framemem-backed containers during declaration.
    // Pushing 128 elements triggers multiple reallocs that break LIFO ordering.
    // Framegraph recompilation should not see any issues with that, until user's framemem usage is correct
    eastl::vector<int, framemem_allocator> progression;
    for (int i = 0; i < N; ++i)
      progression.push_back(i);

    int sum = 0;
    for (int v : progression)
      sum += v;

    return [sum] { CHECK(sum == N * (N - 1) / 2); };
  });

  FRAMEMEM_VALIDATE;
  testRuntime.executeGraph();
}
