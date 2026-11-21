# Original authors, upstream projects, and acknowledgments

The experimental console packages in this branch **modify and rebuild existing community ports** of *Final Fantasy Dimensions*. They are **not original ports** and must not be presented as independent replacements for the authors' work.

## The original ports and mod that make these builds possible

| Original author / maintainer | Original work | How this preview relates |
| --- | --- | --- |
| **[NaGaa95](https://github.com/NaGaa95)** | **[ffd_nx — Final Fantasy Dimensions for Nintendo Switch](https://github.com/NaGaa95/ffd_nx)** | The **entire Switch game wrapper, Android compatibility layer, input handling, renderer integration, OBB loading and NRO build infrastructure** are based on the original Switch port. This preview applies an experimental source patch on top of that project. |
| **[Rinnegatamante](https://github.com/Rinnegatamante)** | **[ffd-vita — Final Fantasy Dimensions for PS Vita](https://github.com/Rinnegatamante/ffd-vita)** | The **entire Vita Android ARMv6 loader, platform integration, rendering, input path, and VPK project** are from the original Vita port. This preview applies experimental loader additions and build-compatibility patches on top. |
| **[fflmod](https://github.com/fflmod)** | **[ffl_mod — original Android mod](https://github.com/fflmod/ffl_mod)** | The **game-aware controller/menu navigation and true-widescreen design** were developed in the upstream Android mod. The portable algorithms, interface hooks, and experimental console rendering adaptations here are derived from that work. The [sourmelee fork](https://github.com/sourmelee/ffl_mod) is a derivative, not the original FFLmod project. |

## Foundational and upstream credits

These acknowledgments are preserved from the original port READMEs and license files, and refer to the original projects' contributions rather than implying that all these individuals participated in this experimental preview:

- **[fgsfds](https://github.com/fgsfdsfgs)** — author of [max_nx](https://github.com/fgsfdsfgs/max_nx), the Switch loader foundation explicitly cited by NaGaa95.
- **[TheOfficialFloW](https://github.com/TheOfficialFloW)** (credited as **TheFloW** in the Vita README) — foundational Android shared-library loading and the Vita homebrew porting technique, acknowledged by both original console projects; see [kubridge](https://github.com/TheOfficialFloW/kubridge) and [FdFix](https://github.com/TheOfficialFloW/FdFix).
- **[frangarcj](https://github.com/frangarcj)** — [Final Fantasy V Vita port (ff5_vita)](https://github.com/frangarcj/ff5_vita), from which the original ffd-vita credits say code was reused.
- **withLogic** — testing and screenshots for the original Vita project, as credited in Rinnegatamante's README.
- **Hans** — testing the original Vita project, as credited in Rinnegatamante's README.
- **Brandonheat8** — original Vita LiveArea assets, as credited in Rinnegatamante's README.
- **Andy Nguyen** — named copyright holder in the original Vita loader's MIT license and in the Switch/max_nx MIT license (alongside fgsfds). Preserve the copyright notices even where a project credits a different maintainer.

## Tools and ecosystem

Many independent developers maintain the tools and dependencies used by the underlying ports. These include [devkitPro/libnx](https://github.com/switchbrew/libnx) and [devkitPro](https://devkitpro.org/) for Nintendo Switch; [VitaSDK](https://github.com/vitasdk), [vitaGL](https://github.com/Rinnegatamante/vitaGL), [vitaShaRK](https://github.com/Rinnegatamante/vitaShaRK), [SoLoud](https://github.com/jarikomppa/soloud), [math-neon](https://github.com/Rinnegatamante/math-neon), and the listed Vita kernel components. Their respective original authors and maintainers retain their rights. The ports also rely on other software libraries whose licenses remain applicable.

## Rights and attribution

- The source included in the original projects is subject to their own licenses; see **[PORTABLE_CONSOLE_NOTICES.txt](PORTABLE_CONSOLE_NOTICES.txt)** for the copyright and MIT license notices corresponding to the three primary source projects, as well as relevant foundational loader notices.
- **SQUARE ENIX CO., LTD.** owns the *Final Fantasy Dimensions* game and associated original intellectual property. This preview is unofficial and is not endorsed by Square Enix or by the upstream port/mod authors.
- The console release packages **do not include** the proprietary game engine `libjniproxy.so` or game OBB assets. Use legally obtained copies with the supported console loaders.
- Retain these acknowledgments and the original copyright notices in redistributions of the modified loaders. When distributing compiled packages, link the upstream source and supply the corresponding license notices.

## Experimental enhancement authorship

The experimental controller/widescreen adaptation, compatibility profiles, patch scripts, and CI release automation were prepared on the [sourmelee/ffl_mod feature branch](https://github.com/sourmelee/ffl_mod/tree/feature/portable-controller-widescreen) using the upstream code as a foundation. The published console files are **cross-compiled experimental builds**; successful compilation does not demonstrate on-device operation.

Please direct appreciation, contributions, and bug reports regarding each original port to the appropriate upstream repository as well.
