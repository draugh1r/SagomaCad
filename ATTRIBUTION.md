# Asset e dipendenze

Il motore di layout [Clay](https://github.com/nicbarker/clay/tree/v0.14), versione 0.14,
è installato da vcpkg, fissato dal `builtin-baseline` del manifest. Licenza zlib,
Copyright (c) 2024 Nic Barker. Il testo completo è distribuito in
`share/clay/copyright` del pacchetto vcpkg; nessun sorgente Clay è modificato.

M0 usa il font [Inter](https://github.com/google/fonts/tree/main/ofl/inter), licenza SIL Open Font License 1.1. La licenza completa è in `assets/fonts/OFL-Inter.txt`.

Le librerie SDL3, Dear ImGui, nlohmann/json e stb sono installate tramite vcpkg secondo le rispettive licenze.
Eigen 5 è installato tramite vcpkg sotto licenza MPL-2.0 ed è usato dal solver di schizzi (Levenberg–Marquardt e decomposizione SVD). Nessun sorgente Eigen è copiato o modificato nel repository.
OpenCascade è installato tramite vcpkg sotto LGPL 2.1 con eccezione OCCT; il testo della licenza è distribuito dal relativo pacchetto vcpkg.

Le icone della UI provengono da [Lucide](https://lucide.dev/), commit `70562c1ee1c4fdcf736fe97bc893fb8511927934`, con licenza ISC e componenti derivate da Feather sotto MIT. Il testo della licenza è in `assets/icons/LUCIDE-LICENSE.txt`. Gli SVG sono rasterizzati localmente tramite LunaSVG, installato con vcpkg sotto licenza MIT.
