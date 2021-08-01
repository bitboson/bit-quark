/* This file is part of bit-quark.
 *
 * Copyright (c) BitBoson
 *
 * bit-quark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bit-quark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bit-quark.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Written by:
 *     - Tyler Parcell <OriginLegend>
 */

//
// JetBrains Space Automation
// This Kotlin-script file lets you automate build activities
// For more info, see https://www.jetbrains.com/help/space/automation.html
//

// Build the project using the higgs-boson build system
job("Build the project using the higgs-boson build system") {

    // Build the project using the higgs-boson build system: Default Linux Binaries
    container(displayName = "Build the default bit-quark Linux Binaries",
              image = "bitboson.registry.jetbrains.space/p/build-tools/build-tools/higgs-boson-builder:newest") {
        shellScript {
            content = """
                higgs-boson download internal
                higgs-boson build-deps internal default
                higgs-boson build internal default
            """
        }
    }
}
