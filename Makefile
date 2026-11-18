.DEFAULT_GOAL := all
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
ARM_CXX := $(DEVKITARM)/bin/arm-none-eabi-g++
HOST_CXX ?= clang++
PYTHON ?= $(if $(wildcard .venv/bin/python),.venv/bin/python,python3)
VERSION := $(shell sed -n 's/.*version = "\([^"]*\)";.*/\1/p' include/app_version.hpp)
COMMON := source/chess/clock.cpp source/chess/chess.cpp source/ai/search.cpp source/ai/styles.cpp source/ai/bots.cpp source/economy/profile.cpp source/storage/store.cpp source/storage/profiles.cpp source/progression/progression.cpp source/openings/openings.cpp source/variants/variants.cpp source/composer/composer.cpp source/platform/battery.cpp source/localization/localization.cpp
APP := $(COMMON) source/space/secret.cpp source/ai/worker.cpp source/ai/hints.cpp source/ui/end_scene.cpp source/ui/onboarding.cpp source/ui/review.cpp source/training/campaign.cpp source/training/academy.cpp source/graphics/renderer.cpp source/graphics/audio.cpp source/tournament/tournament.cpp source/archive/archive.cpp source/ui/preferences.cpp source/ui/developer.cpp source/ui/profiles.cpp source/ui/progression.cpp source/ui/openings.cpp source/ui/variants_ui.cpp source/ui/composer_ui.cpp source/ui/app.cpp source/main.cpp
OBJS := $(patsubst source/%.cpp,build/3ds/%.o,$(APP))
HEADERS := $(shell find include -name '*.hpp' -o -name '*.inc')
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -mword-relocations
CPPFLAGS := -D__3DS__ -Iinclude -Iassets/generated -Iassets/portraits -I$(DEVKITPRO)/libctru/include
CXXFLAGS := $(ARCH) -std=gnu++17 -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti
LDFLAGS := $(ARCH) -specs=tools/3dsx-armv6k.specs -Wl,--gc-sections -Wl,-Map,build/3ds-chess.map -L$(DEVKITPRO)/libctru/lib
HOSTFLAGS := -std=c++17 -O2 -g -Wall -Wextra -pthread -Iinclude -Iassets/generated -Iassets/portraits
.PHONY: all test assets assets-personal portraits clean dist preview sanitize sanitize-polish sanitize-bots sanitize-clocks differential check-tools
all: dist/3ds-chess.3dsx
check-tools:
	@test -x '$(ARM_CXX)' || (echo 'Install devkitPro 3ds-dev: https://devkitpro.org/wiki/Getting_Started'; exit 1)
	@test -f '$(DEVKITPRO)/libctru/lib/libcitro2d.a' || (echo 'Citro2D missing'; exit 1)
build/3ds/%.o: source/%.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp | check-tools
	@mkdir -p '$(@D)'
	'$(ARM_CXX)' $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c '$<' -o '$@'
build/3ds-chess.elf: $(OBJS) Makefile tools/3dsx-armv6k.specs
	'$(ARM_CXX)' $(LDFLAGS) $(OBJS) -lcitro2d -lcitro3d -lctru -lm -o '$@'
dist/3ds-chess.smdh: assets/generated/icon.png Makefile include/app_version.hpp
	@mkdir -p dist
	'$(DEVKITPRO)/tools/bin/smdhtool' --create '3DS Chess' '$(VERSION) / 40 rivales' 'Pollitopio' 'assets/generated/icon.png' '$@'
dist/3ds-chess.3dsx: build/3ds-chess.elf dist/3ds-chess.smdh
	'$(DEVKITPRO)/tools/bin/3dsxtool' '$<' '$@' --smdh=dist/3ds-chess.smdh
build/test_polish_ui: $(APP:source/main.cpp=) tests/test_polish_ui.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_polish_ui.cpp -o '$@'
build/test_hints: $(APP:source/main.cpp=) tests/test_hints.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_hints.cpp -o '$@'
build/test_end_scene: $(APP:source/main.cpp=) tests/test_end_scene.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_end_scene.cpp -o '$@'
build/test_clock: $(COMMON) tests/test_clock.cpp $(HEADERS)
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) tests/test_clock.cpp -o '$@'
build/test_clock_ui: $(APP:source/main.cpp=) tests/test_clock_ui.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_clock_ui.cpp -o '$@'
build/test_core: $(COMMON) tests/test_core.cpp $(HEADERS)
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) tests/test_core.cpp -o '$@'
build/test_storage: $(COMMON) tests/test_storage.cpp $(HEADERS)
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) tests/test_storage.cpp -o '$@'
build/test_app: $(APP:source/main.cpp=) tests/test_app.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_app.cpp -o '$@'
build/test_bots_ui: $(APP:source/main.cpp=) tests/test_bots_ui.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_bots_ui.cpp -o '$@'
build/test_strength: $(COMMON) tests/test_strength.cpp $(HEADERS)
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) tests/test_strength.cpp -o '$@'
build/test_match: $(COMMON) tests/test_match.cpp $(HEADERS)
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) tests/test_match.cpp -o '$@'
test: test-learning build/test_polish_ui build/test_hints build/test_end_scene build/test_clock build/test_clock_ui build/test_core build/test_storage build/test_app build/test_match build/test_strength build/test_bots_ui
	./build/test_polish_ui
	./build/test_hints
	./build/test_end_scene
	./build/test_clock
	./build/test_clock_ui
	./build/test_core
	./build/test_storage
	./build/test_app
	./build/test_match
	./build/test_strength
	./build/test_bots_ui
assets:
	$(PYTHON) tools/prepare_assets.py
portraits:
	$(PYTHON) tools/prepare_portraits.py
assets-personal:
	$(PYTHON) tools/prepare_assets.py --personal-pack
preview: build/test_app build/test_bots_ui build/test_clock_ui build/test_polish_ui
	./build/test_app --preview
	./build/test_bots_ui
	./build/test_clock_ui
	./build/test_polish_ui
	$(PYTHON) tools/render_previews.py
sanitize-polish:
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_polish_ui.cpp -o build/sanitize_polish_ui
	./build/sanitize_polish_ui
sanitize-bots:
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_bots_ui.cpp -o build/sanitize_bots_ui
	./build/sanitize_bots_ui
sanitize-clocks:
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_clock_ui.cpp -o build/sanitize_clock_ui
	./build/sanitize_clock_ui
sanitize:
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(COMMON) tests/test_core.cpp -o build/sanitize_core
	./build/sanitize_core
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_app.cpp -o build/sanitize_app
	./build/sanitize_app
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(COMMON) tests/test_strength.cpp -o build/sanitize_strength
	./build/sanitize_strength
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_bots_ui.cpp -o build/sanitize_bots_ui
	./build/sanitize_bots_ui
differential: build/rules_cli
	$(PYTHON) tests/differential.py
build/rules_cli: source/chess/chess.cpp tests/rules_cli.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) source/chess/chess.cpp tests/rules_cli.cpp -o '$@'
dist: all test
	$(PYTHON) tools/package.py --local-testing
clean:
	rm -rf build/3ds build/3ds-chess.elf build/3ds-chess.map build/test_polish_ui build/test_hints build/test_end_scene build/test_clock build/test_clock_ui build/test_core build/test_storage build/test_app build/test_match build/test_strength build/test_bots_ui build/rules_cli build/sanitize_polish_ui build/sanitize_clock_ui build/sanitize_core build/sanitize_app build/sanitize_strength build/sanitize_bots_ui dist/3ds-chess.3dsx dist/3ds-chess.smdh
-include $(OBJS:.o=.d)

build/test_review: $(APP:source/main.cpp=) tests/test_review.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_review.cpp -o '$@'

build/test_training: $(APP:source/main.cpp=) tests/test_training.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_training.cpp -o '$@'

build/test_audio_dialogue: $(APP:source/main.cpp=) tests/test_audio_dialogue.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_audio_dialogue.cpp -o '$@'

build/test_learning_ui: $(APP:source/main.cpp=) tests/test_learning_ui.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_learning_ui.cpp -o '$@'

.PHONY: test-learning sanitize-learning
test-learning: build/test_review build/test_training build/test_audio_dialogue build/test_learning_ui
	./build/test_review
	./build/test_training
	./build/test_audio_dialogue
	./build/test_learning_ui
sanitize-learning:
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_learning_ui.cpp -o build/sanitize_learning_ui
	./build/sanitize_learning_ui

AUDIT_TESTS := audit_ui worker_stress storage_audit
$(addprefix build/test_,$(AUDIT_TESTS)): build/test_%: $(APP:source/main.cpp=) tests/test_%.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_$*.cpp -o '$@'
.PHONY: test-audit sanitize-audit
test-audit: $(addprefix build/test_,$(AUDIT_TESTS))
	./build/test_audit_ui
	./build/test_worker_stress
	./build/test_storage_audit
test: test-audit
sanitize-audit:
	@mkdir -p build
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_audit_ui.cpp -o build/sanitize_audit_ui
	./build/sanitize_audit_ui
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_worker_stress.cpp -o build/sanitize_worker_stress
	./build/sanitize_worker_stress

build/test_native_resources: source/graphics/renderer.cpp source/graphics/audio.cpp source/ai/bots.cpp source/chess/chess.cpp source/localization/localization.cpp tests/test_native_resources.cpp tests/native_stubs/3ds.h tests/native_stubs/citro2d.h $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) -D__3DS__ -Itests/native_stubs source/graphics/renderer.cpp source/graphics/audio.cpp source/ai/bots.cpp source/chess/chess.cpp source/localization/localization.cpp tests/test_native_resources.cpp -o '$@'
test-native: build/test_native_resources
	./build/test_native_resources
test-audit: test-native
sanitize-native:
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -D__3DS__ -Itests/native_stubs source/graphics/renderer.cpp source/graphics/audio.cpp source/ai/bots.cpp source/chess/chess.cpp source/localization/localization.cpp tests/test_native_resources.cpp -o build/sanitize_native_resources
	./build/sanitize_native_resources

.PHONY: test-native sanitize-native sanitize-storage
sanitize-storage:
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_storage_audit.cpp -o build/sanitize_storage_audit
	./build/sanitize_storage_audit

.PHONY: test-package audit-assets
test-package:
	$(PYTHON) tests/test_packaging.py
audit-assets:
	$(PYTHON) tools/audit_assets.py
test-audit: test-package audit-assets

build/test_preferences: $(APP:source/main.cpp=) tests/test_preferences.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_preferences.cpp -o '$@'

build/test_tournament: $(COMMON) source/ai/worker.cpp source/tournament/tournament.cpp tests/test_tournament.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) source/ai/worker.cpp source/tournament/tournament.cpp tests/test_tournament.cpp -o '$@'
build/test_archive: $(COMMON) source/archive/archive.cpp tests/test_archive.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) source/archive/archive.cpp tests/test_archive.cpp -o '$@'
build/test_update_ui: $(APP:source/main.cpp=) tests/test_update_ui.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_update_ui.cpp -o '$@'
test-update: build/test_preferences build/test_tournament build/test_archive build/test_update_ui

build/test_academy: source/chess/chess.cpp source/training/academy.cpp tests/test_academy.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) source/chess/chess.cpp source/training/academy.cpp tests/test_academy.cpp -o '$@'
build/test_styles: source/chess/chess.cpp source/ai/search.cpp source/ai/styles.cpp tests/test_styles.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) source/chess/chess.cpp source/ai/search.cpp source/ai/styles.cpp tests/test_styles.cpp -o '$@'
test-update: build/test_academy build/test_styles
	./build/test_preferences
	./build/test_tournament
	./build/test_archive
	./build/test_update_ui
	./build/test_academy
	./build/test_styles
.PHONY: test-update sanitize-update
sanitize-update:
	@mkdir -p build/update-090
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_update_ui.cpp -o build/update-090/sanitize_update_ui
	./build/update-090/sanitize_update_ui
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_preferences.cpp -o build/update-090/sanitize_preferences
	./build/update-090/sanitize_preferences

test: test-update

CONTENT := $(COMMON) source/ai/worker.cpp source/training/academy.cpp source/tournament/tournament.cpp source/archive/archive.cpp
build/update-090/sanitize_%: tests/test_%.cpp $(CONTENT) $(HEADERS)
	@mkdir -p '$(@D)'
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(CONTENT) '$<' -o '$@'
.PHONY: sanitize-content
sanitize-content: build/update-090/sanitize_academy build/update-090/sanitize_styles build/update-090/sanitize_tournament build/update-090/sanitize_archive
	./build/update-090/sanitize_academy
	./build/update-090/sanitize_styles
	./build/update-090/sanitize_tournament
	./build/update-090/sanitize_archive

.PHONY: test-target
test-target: all
	$(PYTHON) tools/check_target.py

build/test_navigation: $(APP:source/main.cpp=) tests/test_navigation.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_navigation.cpp -o '$@'
.PHONY: test-navigation sanitize-navigation
test-navigation: build/test_navigation
	./build/test_navigation
test: test-navigation
sanitize-navigation:
	@mkdir -p build/navigation-audit-090
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_navigation.cpp -o build/navigation-audit-090/sanitize_navigation
	./build/navigation-audit-090/sanitize_navigation

LEGACY_TESTS := profiles legacy_ui
$(addprefix build/test_,$(LEGACY_TESTS)): build/test_%: $(APP:source/main.cpp=) tests/test_%.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_$*.cpp -o '$@'
test-legacy: $(addprefix build/test_,$(LEGACY_TESTS))
	./build/test_profiles
	./build/test_legacy_ui
test: test-legacy
sanitize-legacy:
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_legacy_ui.cpp -o build/legacy-update/sanitize_ui
	./build/legacy-update/sanitize_ui

PROGRESSION_TESTS := progression progression_ui
$(addprefix build/test_,$(PROGRESSION_TESTS)): build/test_%: $(APP:source/main.cpp=) tests/test_%.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_$*.cpp -o '$@'
test-progression: $(addprefix build/test_,$(PROGRESSION_TESTS))
	./build/test_progression
	./build/test_progression_ui
test: test-progression
sanitize-progression:
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_progression_ui.cpp -o build/legacy-update/sanitize_progression_ui
	./build/legacy-update/sanitize_progression_ui

OPENINGS_TESTS := openings openings_ui
$(addprefix build/test_,$(OPENINGS_TESTS)): build/test_%: $(APP:source/main.cpp=) tests/test_%.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_$*.cpp -o '$@'
test-openings: $(addprefix build/test_,$(OPENINGS_TESTS))
	./build/test_openings
	./build/test_openings_ui
test: test-openings
sanitize-openings:
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_openings_ui.cpp -o build/legacy-update/sanitize_openings_ui
	./build/legacy-update/sanitize_openings_ui

# Legacy remaining modules and UI regression coverage.
build/test_variants: $(COMMON) source/ai/worker.cpp tests/test_variants.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) source/ai/worker.cpp tests/test_variants.cpp -o '$@'
build/test_composer: $(COMMON) tests/test_composer.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) tests/test_composer.cpp -o '$@'
build/test_battery: source/platform/battery.cpp tests/test_battery.cpp include/platform/battery.hpp
	$(HOST_CXX) $(HOSTFLAGS) source/platform/battery.cpp tests/test_battery.cpp -o '$@'
build/test_composer_ui: $(APP:source/main.cpp=) tests/test_composer_ui.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_composer_ui.cpp -o '$@'
build/test_variants_ui: $(APP:source/main.cpp=) tests/test_variants_ui.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_variants_ui.cpp -o '$@'
test-remaining: build/test_variants build/test_composer build/test_battery build/test_composer_ui build/test_variants_ui
	./build/test_variants
	./build/test_composer
	./build/test_battery
	./build/test_composer_ui
	./build/test_variants_ui
test: test-remaining
sanitize-remaining:
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_variants_ui.cpp -o build/sanitize_variants_ui
	./build/sanitize_variants_ui
	$(HOST_CXX) $(HOSTFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(APP:source/main.cpp=) tests/test_composer_ui.cpp -o build/sanitize_composer_ui
	./build/sanitize_composer_ui

build/test_translation: source/localization/localization.cpp tests/test_translation.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) source/localization/localization.cpp tests/test_translation.cpp -o '$@'
test-translations: build/test_translation
	./build/test_translation
	$(PYTHON) tests/test_localization_catalog.py
	$(PYTHON) tests/test_content_catalog.py
test: test-translations

build/test_space_secret: $(APP:source/main.cpp=) tests/test_space_secret.cpp $(HEADERS)
	$(HOST_CXX) $(HOSTFLAGS) $(APP:source/main.cpp=) tests/test_space_secret.cpp -o '$@'
build/test_space_scene: $(COMMON) source/ui/end_scene.cpp source/graphics/renderer.cpp tests/test_space_scene.cpp $(HEADERS) assets/generated/atlas.hpp assets/portraits/portraits.hpp
	$(HOST_CXX) $(HOSTFLAGS) $(COMMON) source/ui/end_scene.cpp source/graphics/renderer.cpp tests/test_space_scene.cpp -o '$@'
test-space-modules: build/test_space_secret build/test_space_scene
	./build/test_space_secret
	./build/test_space_scene
test: test-space-modules
