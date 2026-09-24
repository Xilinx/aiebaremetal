/**
 * @file test_generate_event.cpp
 * @brief Unit tests for AIE event generation helpers
 *
 * This file contains unit tests for functions that generate events on various AIE
 * tile types (shim, core, memory) and across columns. It verifies that the correct
 * XAie events are issued and that error and user-defined event sequences are triggered
 * as expected.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#include "stubs.h"

#include "xaie_generate_events.h"
#include "test_comparator.h"

/**
 * @brief Test group for shim tile event generation functionality
 *
 * This test group verifies that generate_shim_tile_event issues XAie events for all
 * shim-related event groups when the tile type is a supported shim type (e.g.,
 * XAIEGBL_TILE_TYPE_SHIMNOC or XAIEGBL_TILE_TYPE_SHIMPL), and that non-shim tile
 * types take the early-exit path without generating shim events.
 */
TEST_GROUP(GenerateShimTileEventTests)
{
	// Test fixtures
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes the mock XAie device instance and test tile location used by all
	 * generate_shim_tile_event test cases.
	 */
	void setup()
	{
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Cleans up the mock XAie device instance, test tile location, and clears
	 * mock expectations.
	 */
	void teardown()
	{
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Verify valid shim NoC tile generates all expected events
 *
 * This test verifies that generate_shim_tile_event issues XAie events for all shim
 * event groups when the tile type is XAIEGBL_TILE_TYPE_SHIMNOC.
 */
TEST(GenerateShimTileEventTests, TC_001_ValidShimNocTile)
{
	// ARRANGE: Setup shim NoC tile type
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_shim_tile_event((XAie_DevInst*)&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_002: Verify valid shim PL tile generates all expected events
 *
 * This test verifies that generate_shim_tile_event behaves identically for a
 * XAIEGBL_TILE_TYPE_SHIMPL tile type.
 */
TEST(GenerateShimTileEventTests, TC_002_WorksWithShimPlTile)
{
	// ARRANGE: Setup shim PL tile type
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMPL);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_shim_tile_event((XAie_DevInst*)&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_003: Verify non-shim tile type takes early-exit path
 *
 * This test verifies that generate_shim_tile_event handles invalid/non-shim tile
 * types correctly by taking the early-exit path.
 */
TEST(GenerateShimTileEventTests, TC_003_ExitsEarlyWithNonShimTile)
{
	// ARRANGE: Setup invalid tile type
	Stub_XAie_SetTileType(99);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}
	mock_c()->expectOneCall("XAie_EventGenerate")
		->ignoreOtherParameters();

	// ACT: Call the function under test
	generate_shim_tile_event((XAie_DevInst*)&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief Test group for AIE tile memory event generation functionality
 *
 * This test group verifies that generate_aie_tile_mem_event generates memory-related
 * events for XAIEGBL_TILE_TYPE_AIETILE tiles and handles non-AIE tile types gracefully.
 */
TEST_GROUP(GenerateAieTileMemEventTests)
{
	// Test fixtures
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Configures the mock XAie device instance and tile type as XAIEGBL_TILE_TYPE_AIETILE,
	 * and initializes the tile location fixture.
	 */
	void setup()
	{
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Releases resources and resets the mock device instance and location.
	 */
	void teardown()
	{
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Verify successful event generation for AIE tile memory
 *
 * This test verifies that generate_aie_tile_mem_event issues XAie events for each
 * memory-related event group when tile type is XAIEGBL_TILE_TYPE_AIETILE.
 */
TEST(GenerateAieTileMemEventTests, TC_001_SuccessfulEventGeneration)
{
	// ARRANGE: Setup AIE tile type
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_AIETILE);
	for (int i = XAIE_EVENT_GROUP_ERRORS_MEM + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_aie_tile_mem_event(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}
/**
 * @brief TC_002: Verify non-AIE tile type is handled gracefully
 *
 * This test verifies that generate_aie_tile_mem_event handles invalid/non-AIE tile
 * types without crashing.
 */
TEST(GenerateAieTileMemEventTests, TC_002_NonAieTile)
{
	// ARRANGE: Setup invalid tile type
	Stub_XAie_SetTileType(99);
	for (int i = XAIE_EVENT_GROUP_ERRORS_MEM + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_aie_tile_mem_event(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief Test group for memory tile event generation functionality
 *
 * This test group verifies that generate_mem_tile_event generates memory-tile-specific
 * events for XAIEGBL_TILE_TYPE_MEMTILE tiles.
 */
TEST_GROUP(GenerateMemTileEventTests)
{
	// Test fixtures
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes the mock XAie device instance with XAIEGBL_TILE_TYPE_MEMTILE tile type
	 * and prepares the tile location.
	 */
	void setup()
	{
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Cleans up the memtile-specific fixtures and resets modified state.
	 */
	void teardown()
	{
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify successful event generation for memory tile
 *
 * This test verifies that generate_mem_tile_event issues XAie events for each
 * memory-tile-specific event group when tile type is XAIEGBL_TILE_TYPE_MEMTILE.
 */
TEST(GenerateMemTileEventTests, TC_001_SuccessfulEventGeneration)
{
	// ARRANGE: Setup memory tile type
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_MEMTILE);
	for (int i = XAIE_EVENT_GROUP_ERRORS_MEM_TILE + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM_TILE; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_mem_tile_event(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}
/**
 * @brief TC_002: Verify non-memory tile type is handled gracefully
 *
 * This test verifies that generate_mem_tile_event handles invalid/non-memory tile
 * types without crashing.
 */
TEST(GenerateMemTileEventTests, TC_002_NonAieTile)
{
	// ARRANGE: Setup invalid tile type
	Stub_XAie_SetTileType(99);
	for (int i = XAIE_EVENT_GROUP_ERRORS_MEM_TILE + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM_TILE; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_mem_tile_event(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief Test group for AIE tile core event generation functionality
 *
 * This test group verifies that generate_aie_tile_core_event triggers the expected
 * core-related event groups for XAIEGBL_TILE_TYPE_AIETILE tiles.
 */
TEST_GROUP(GenerateAieTileCoreEventTests)
{
	// Test fixtures
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Sets up the mock XAie device instance and configures the tile as
	 * XAIEGBL_TILE_TYPE_AIETILE.
	 */
	void setup()
	{
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Tears down the AIE core test fixtures and resets state.
	 */
	void teardown()
	{
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Verify successful event generation for AIE core tile
 *
 * This test verifies that generate_aie_tile_core_event issues XAie events for all
 * core-related event groups when tile type is XAIEGBL_TILE_TYPE_AIETILE.
 */
TEST(GenerateAieTileCoreEventTests, TC_001_SuccessfulEventGeneration)
{
	// ARRANGE: Setup AIE tile type
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_AIETILE);
	for (int i = XAIE_EVENT_GROUP_ERRORS_1_CORE + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_CORE; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_aie_tile_core_event(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}
/**
 * @brief TC_002: Verify non-AIE tile type is handled gracefully
 *
 * This test verifies that generate_aie_tile_core_event handles invalid/non-core tile
 * types without errors.
 */
TEST(GenerateAieTileCoreEventTests, TC_002_NonAieTile)
{
	// ARRANGE: Setup invalid tile type
	Stub_XAie_SetTileType(99);
	for (int i = XAIE_EVENT_GROUP_ERRORS_1_CORE + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_CORE; i++)
	{
		mock_c()->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_aie_tile_core_event(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}


/**
 * @brief Test group for column-wide event generation functionality
 *
 * This test group verifies that generate_events_on_col and related helpers generate
 * the expected shim, AIE, and memtile events across all tiles in a column.
 */
TEST_GROUP(GenerateEventsOnColTests)
{
	// Test fixtures
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes the mock XAie device instance and configures tile types and locations
	 * for a representative column.
	 */
	void setup()
	{
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Cleans up column fixtures and resets modified state.
	 */
	void teardown()
	{
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify all shim tiles in column generate shim events
 *
 * This test verifies that generate_events_on_col calls generate_shim_tile_event for
 * shim tiles in the column.
 */
TEST(GenerateEventsOnColTests, TC_001_AllShimTiles)
{
	// ARRANGE
	const int testCol = 3; // Use middle column for this test
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	mock_c()->expectOneCall("generate_shim_tile_event")
		->withPointerParameters("DevInst", &mockDevInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL ; i++) {
		mock_c()->expectOneCall("XAie_EventGenerate")
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_events_on_col(&mockDevInst, testCol);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_002: All mem tiles in a column generate memory events.
 *
 * Uses generate_events_on_col with a column index whose tiles are treated
 * as memory tiles (XAIEGBL_TILE_TYPE_MEMTILE). The test checks that
 * generate_mem_tile_event and XAie_EventGenerate are invoked for all
 * memory event groups for that column.
 */
TEST(GenerateEventsOnColTests, TC_002_AllMemTiles)
{
	// ARRANGE
	const int testCol = 4; // Use different column for this test
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_MEMTILE);
	mock_c()->expectOneCall("generate_mem_tile_event")
		->withPointerParameters("DevInst", &mockDevInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc);
	for(int i = XAIE_EVENT_GROUP_ERRORS_MEM  + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++) {
		mock_c()->expectOneCall("XAie_EventGenerate")
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_events_on_col(&mockDevInst, testCol);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_003: All AIE tiles in a column generate core and memory events.
 *
 * Iterates over all rows in a column of XAIEGBL_TILE_TYPE_AIETILE tiles and
 * calls generate_events_on_col. The test expects both
 * generate_aie_tile_core_event and generate_aie_tile_mem_event to be
 * invoked per tile, along with XAie_EventGenerate calls for all related
 * core and memory event groups.
 */
TEST(GenerateEventsOnColTests, TC_003_AllAIETiles)
{
	// ARRANGE
	const int testCol = 5; // Use different column for this test
	const int NUM_ROWS_MOCKED = mockDevInst.NumRows;
	XAie_LocType currentLoc;
	for (int row = 0; row < NUM_ROWS_MOCKED; row++)
	{
		currentLoc.Col = testCol;
		currentLoc.Row = row;
		Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_AIETILE);
		mock_c()->expectOneCall("generate_aie_tile_core_event")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc);
		for (int i = XAIE_EVENT_GROUP_ERRORS_1_CORE   + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_CORE; i++) {
			mock_c()->expectOneCall("XAie_EventGenerate")
				->withPointerParameters("DevInst", &mockDevInst)
				->ignoreOtherParameters()
				->andReturnIntValue(XAIE_OK);
		}
		mock_c()->expectOneCall("generate_aie_tile_mem_event")
			->withPointerParameters("DevInst", &mockDevInst)
			->withParameterOfType("XAie_LocType", "Loc", &testLoc);
		for(int i = XAIE_EVENT_GROUP_ERRORS_MEM  + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++) {
			mock_c()->expectOneCall("XAie_EventGenerate")
				->withPointerParameters("DevInst", &mockDevInst)
				->ignoreOtherParameters()
				->andReturnIntValue(XAIE_OK);
		}
	}
	// ACT: Call the function under test
	generate_events_on_col(&mockDevInst, testCol);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_004: Column containing invalid tile types is handled safely.
 *
 * Configures tiles in the target column with an invalid tile type value
 * and calls generate_events_on_col. The test verifies that the helper
 * does not crash and completes without generating unexpected events.
 */
TEST(GenerateEventsOnColTests, TC_004_AllInvalidTileTypes)
{
	// ARRANGE
	const int testCol = 6; // Use different column for this test
	Stub_XAie_SetTileType(99);
	// ACT: Call the function under test
	generate_events_on_col(&mockDevInst, testCol);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_005: Column of shim PL tiles generates shim events.
 *
 * Treats tiles in the selected column as XAIEGBL_TILE_TYPE_SHIMPL and
 * calls generate_events_on_col. The test ensures that
 * generate_shim_tile_event and the shim-related XAie_EventGenerate calls
 * are performed for all relevant shim event groups.
 */
TEST(GenerateEventsOnColTests, TC_005_AllShimTiles)
{
	// ARRANGE
	const int testCol = 3; // Use middle column for this test
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMPL);
	mock_c()->expectOneCall("generate_shim_tile_event")
		->withPointerParameters("DevInst", &mockDevInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL ; i++) {
		mock_c()->expectOneCall("XAie_EventGenerate")
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}

	// ACT: Call the function under test
	generate_events_on_col(&mockDevInst, testCol);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief Test group for user event 1 generation on a column.
 *
 * Verifies that generate_user_event1_on_col triggers the expected user
 * event across tiles in a selected column. The tests cover nominal and
 * boundary cases (minimum and maximum column indices) to ensure correct
 * behavior for all valid columns.
 */
TEST_GROUP(GenerateUserEvent1OnColTests)
{
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Common setup for user event 1 column tests.
	 *
	 * Configures the mock XAie device instance and initializes the column
	 * index and tile locations used by generate_user_event1_on_col test
	 * cases. Also prepares any expected event masks.
	 */
	void setup() {
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Common teardown for user event 1 column tests.
	 *
	 * Resets the column configuration, mock device instance, and any
	 * allocated data structures used in user event 1 tests.
	 */
	void teardown() {
		// Cleanup after each test if needed
		mock_c()->removeAllComparatorsAndCopiers();
		// Clear mock expectations for clean test state
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Nominal user event1 generation on a valid column.
 *
 * Calls generate_user_event1_on_col with a mid-range valid column index.
 * The test stubs XAie_EventGenerate to succeed and verifies that the
 * function triggers the user event once for the specified column.
 */
TEST(GenerateUserEvent1OnColTests, TC_001_NominalCase)
{
	// ARRANGE
	int col = 3; // Valid column
	mock_c()->expectOneCall("XAie_EventGenerate")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT
	generate_user_event1_on_col(&mockDevInst, col);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_002: User event1 generation on the minimum valid column index.
 *
 * Invokes generate_user_event1_on_col with column 0, the minimum valid
 * index for the configured device. The test checks that the user event is
 * still generated correctly at the boundary.
 */
TEST(GenerateUserEvent1OnColTests, TC_002_MinimumColumn)
{
	// ARRANGE
	int col = 0; // Minimum valid column
	mock_c()->expectOneCall("XAie_EventGenerate")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT
	generate_user_event1_on_col(&mockDevInst, col);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_003: User event1 generation on the maximum valid column index.
 *
 * Calls generate_user_event1_on_col with the last valid column index,
 * derived from mockDevInst->NumCols - 1. The test confirms that the user
 * event is issued successfully at the upper boundary.
 */
TEST(GenerateUserEvent1OnColTests, TC_003_MaximumColumn)
{
	// ARRANGE
	int col = mockDevInst.NumCols - 1; // Maximum valid column
	mock_c()->expectOneCall("XAie_EventGenerate")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT
	generate_user_event1_on_col(&mockDevInst, col);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief Test group for memtile checkbit error generation.
 *
 * Validates generate_mem_tile_checkbit_error_generation for different
 * device generations and tile types. The tests ensure that checkbit
 * error generation only occurs for supported generations and memtile
 * tiles, and that unsupported combinations follow the correct failure
 * path without performing invalid register writes.
 */
TEST_GROUP(GenerateMemTileCheckbitErrorTests)
{
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Common setup for memtile checkbit error tests.
	 *
	 * Initializes the mock XAie device instance, device generation, and
	 * memtile location used to exercise
	 * generate_mem_tile_checkbit_error_generation. Prepares register
	 * addresses and default values if needed.
	 */
	void setup() {
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Common teardown for memtile checkbit error tests.
	 *
	 * Cleans up the memtile error-generation fixtures, including any mock
	 * register programming state and device configuration.
	 */
	void teardown() {
		// Cleanup after each test if needed
		mock_c()->removeAllComparatorsAndCopiers();
		// Clear mock expectations for clean test state
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Failure path for checkbit error generation based on device generation.
 *
 * Sets the device generation (DevGen) such that checkbit error generation
 * is not supported (e.g., XAIE_DEV_GEN_AIE). The test calls
 * generate_mem_tile_checkbit_error_generation and verifies that the
 * function takes the failure path without generating errors or events.
 */
TEST(GenerateMemTileCheckbitErrorTests, TC_001_FailureCheckbitErrorGenerationForGen)
{
	// ARRANGE
	mockDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIE ; // Set gen > AIEML

	// ACT
	generate_mem_tile_checkbit_error_generation(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_002: Failure path when tile type is invalid for checkbit errors.
 *
 * Configures a device generation that supports checkbit errors but sets
 * the tile type to a non-memory type (e.g., XAIEGBL_TILE_TYPE_SHIMPL).
 * The test calls generate_mem_tile_checkbit_error_generation and verifies
 * that the function does not attempt the memory-tile specific sequence.
 */
TEST(GenerateMemTileCheckbitErrorTests, TC_002_FailureCheckbitErrorGenerationForTileType)
{
	// ARRANGE
	mockDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1; // Set gen > AIEML

	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMPL);

	// ACT
	generate_mem_tile_checkbit_error_generation(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief TC_003: Successful checkbit error generation on a valid memtile.
 *
 * Sets DevGen to a value that supports checkbit errors and configures the
 * tile type as XAIEGBL_TILE_TYPE_MEMTILE. The test expects
 * XAie_GetTileAddr and XAie_Write32 to be called with appropriate
 * parameters when generate_mem_tile_checkbit_error_generation is invoked
 * for the test location.
 */
TEST(GenerateMemTileCheckbitErrorTests, TC_003_SuccessCheckbitErrorGenerationForTileType)
{
	// ARRANGE
	mockDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1; // Set gen > AIEML
	const u64 BASE_TILE_ADDR = 0x1000;
	u32 Value = 1;
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_MEMTILE);

	mock_c()->expectOneCall("XAie_Write32")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_GetTileAddr")
		->withPointerParameters("DevInst", &mockDevInst)
		->withUnsignedIntParameters("R", testLoc.Row)
		->withUnsignedIntParameters("C", testLoc.Col)
		->andReturnIntValue(XAIE_OK);
	// ACT
	generate_mem_tile_checkbit_error_generation(&mockDevInst, testLoc);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief Test group for column-based memtile checkbit error generation.
 *
 * Focuses on generate_mem_tile_checkbit_error_generation_col, which applies
 * memtile checkbit error generation across a specified column. The tests
 * verify that the per-tile helper is invoked correctly for each memtile in
 * the column and that invalid columns are handled safely.
 */
TEST_GROUP(GenerateMemTileCheckbitErrorTestsforcol)
{
	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Common setup for column-based memtile checkbit error tests.
	 *
	 * Sets up the mock XAie device instance and configures a test column of
	 * memtiles for generate_mem_tile_checkbit_error_generation_col. This
	 * includes initializing tile locations and any per-column metadata.
	 */
	void setup() {
		RegisterXAieLocTypeComparator();
		// Setup common test infrastructure
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 8;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Common teardown for column-based memtile checkbit error tests.
	 *
	 * Resets the column configuration, mock device instance, and any helper
	 * structures created to track memtiles in the test column.
	 */
	void teardown() {
		// Cleanup after each test if needed
		mock_c()->removeAllComparatorsAndCopiers();
		// Clear mock expectations for clean test state
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Column-based checkbit error generation for memtiles.
 *
 * Calls generate_mem_tile_checkbit_error_generation_col for a specific
 * column index. The test expects generate_mem_tile_checkbit_error_generation
 * to be invoked once with the mock device instance and the provided
 * column, ensuring the column-level helper dispatches correctly.
 */
TEST(GenerateMemTileCheckbitErrorTestsforcol, TC_001_CheckbitErrorGenerationForCol)
{
	// ARRANGE
	int col = 3; // Valid column

	mock_c()->expectOneCall("generate_mem_tile_checkbit_error_generation")
		->withPointerParameters("DevInst", &mockDevInst)
		->withIntParameters("col", col);
	// ACT
	generate_mem_tile_checkbit_error_generation_col(&mockDevInst, col);

	// ASSERT: Verify mock expectations are satisfied
}

/**
 * @brief Test group for AI Engine interrupt generation flow.
 *
 * Validates the top-level interrupt generation sequence implemented by
 * test_aie_generate_intr. The tests exercise both success and failure
 * paths by stubbing XAie_PmRequestTiles, XAie_ErrorHandlingInit, and the
 * event generation helpers, and verify correct status reporting and call
 * ordering.
 */
TEST_GROUP(AieGenerateIntrTests)
{
	XAie_DevInst* devInst = new XAie_DevInst;
	XAie_LocType testLoc;

	/**
	 * @brief Common setup for AIE interrupt generation tests.
	 *
	 * Initializes the mock XAie device instance, tile locations, and any
	 * global configuration required by test_aie_generate_intr. Stubs default
	 * return values for XAie_PmRequestTiles, XAie_ErrorHandlingInit, and
	 * event helper functions unless a test overrides them.
	 */
	void setup()
	{
		RegisterXAieLocTypeComparator();
		// Initialize with default test values
		devInst->NumCols = 1;
		devInst->NumRows = 4;
		devInst->StartCol = 0;
		devInst->MemTileRowStart = 1;
		devInst->MemTileNumRows = 2;
		devInst->DevProp.DevGen = XAIE_DEV_GEN_AIE;
		devInst->DevOps = &s_MockDevOps;
		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	/**
	 * @brief Common teardown for AIE interrupt generation tests.
	 *
	 * Cleans up all interrupt-generation fixtures, including mock functions,
	 * device instance, and global state, to ensure isolation between test
	 * cases.
	 */
	void teardown()
	{
		delete devInst;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Failure cases for AIE interrupt generation.
 *
 * Configures XAie_PmRequestTiles and XAie_ErrorHandlingInit to fail and
 * then calls test_aie_generate_intr. The test asserts that a non-zero
 * status is returned, indicating that error paths in the interrupt
 * generation sequence are handled and propagated correctly.
 */

TEST(AieGenerateIntrTests, TC_001_FailureCases)
{
	mock_c()->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("NumTiles", 0)
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")
		->withPointerParameters("DevInst", devInst)
		->andReturnIntValue(1);
	// ACT
	int result = test_aie_generate_intr(devInst);

	// ASSERT
	CHECK(result != 0); // Non-zero indicates error
}


/**
 * @brief TC_002: Success cases for AIE interrupt generation.
 *
 * Stubs XAie_PmRequestTiles, XAie_ErrorHandlingInit, and the various event
 * generation helpers (generate_user_event1_on_col,
 * generate_mem_tile_checkbit_error_generation, generate_shim_tile_event)
 * to succeed. The test then calls test_aie_generate_intr and verifies that
 * a zero status is returned and that XAie_EventGenerate and the helper
 * functions are invoked with the expected DevInst and Loc parameters.
 */

TEST(AieGenerateIntrTests, TC_002_SuccessCases)
{
	mock_c()->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")
		->withPointerParameters("DevInst", devInst)
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_EventGenerate")
		->withPointerParameters("DevInst", devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("generate_user_event1_on_col")
		->withPointerParameters("DevInst", devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc);
	mock_c()->expectOneCall("generate_mem_tile_checkbit_error_generation")
		->withPointerParameters("DevInst", devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc);
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	mock_c()->expectOneCall("generate_shim_tile_event")
		->withPointerParameters("DevInst", devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL ; i++) {
		mock_c()->expectOneCall("XAie_EventGenerate")
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}
	// ACT
	int result = test_aie_generate_intr(devInst);

	// ASSERT
	CHECK(result == 0);
}
