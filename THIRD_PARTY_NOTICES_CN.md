# 第三方声明

[English](THIRD_PARTY_NOTICES.md) | [简体中文](THIRD_PARTY_NOTICES_CN.md)

项目 MIT 许可证仅适用于项目自有源代码，不替代第三方组件许可证。获取的源码树中的上游许可证文件和 SPDX 声明具有最终效力。

| 组件 | 集成方式 | 版本或提交 | 来源 | 许可证 | 仓库内许可证依据 |
|---|---|---|---|---|---|
| M5PM1 | 由 `fetch_repos.py` 获取 | `be9a5456c007c333e7ac963f33bfde1ffa5d82ee` | https://github.com/m5stack/M5PM1 | MIT | 获取源码树中的上游许可证 |
| cmake_utilities | 由 `fetch_repos.py` 从源码子目录获取 | `05f51bd7044e9f95bc978093fc78656e4d81cd37` | https://github.com/espressif/esp-iot-solution | Apache-2.0 | 获取源码树中的上游许可证 |
| M5IOE1 | 由 `fetch_repos.py` 获取 | `846eec7d05e25c09013be2acdb8804487f48a62e` | https://github.com/m5stack/M5IOE1 | MIT | 获取源码树中的上游许可证 |
| i2c_bus | ESP-IDF Component Manager | 1.5.2（`ef13dcfd5aa18c4d6dca3d89d18173f8ae180a5f`） | https://components.espressif.com/components/espressif/i2c_bus | Apache-2.0 | 组件元数据和获取源码中的许可证 |
| M5GFX | 由 `fetch_repos.py` 获取 | `d91077b9a607b59404e4e4a49f775c792bfae382` | https://github.com/m5stack/M5GFX | MIT | 获取源码树中的上游许可证 |
| BMI270 Sensor API | 获取源码子目录 | `23fb37517f2e1db543fe460ffbcc5716990123dd` | https://github.com/m5stack/M5StopWatch-PowerDemo | BSD-3-Clause | 源码快照中的 `LICENSE` |
| ic_rx8130 | 由 `fetch_repos.py` 获取 | `d79bf99aa25168ab26886d0e94cf525be5a1343e` | https://github.com/Ocean-lhy/ic_rx8130 | MIT | 源码 SPDX 声明；未记录独立许可证文件 |
| RadioLib | 由 `fetch_repos.py` 获取 | `187ef24791c3d844939b2be13a68bd890bd04e4c` | https://github.com/jgromes/RadioLib | MIT | 获取源码树中的上游许可证 |
| M5Unit-NFC | 由 `fetch_repos.py` 获取并应用本地依赖补丁 | `93745b547364f310cd64b5155a870103a7800a5d` | https://github.com/m5stack/M5Unit-NFC | MIT | 获取源码树中的上游许可证 |
| M5UnitUnified | 由 `fetch_repos.py` 获取并应用本地依赖补丁 | `bf711f370047cf16355b00005450ef615fab36e2` | https://github.com/m5stack/M5UnitUnified | MIT | 获取源码树中的上游许可证 |
| M5Utility | 由 `fetch_repos.py` 获取 | `301a6b5c6413875e1dd80b027e0639921972b433` | https://github.com/m5stack/M5Utility | MIT | 获取源码树中的上游许可证 |
| M5HAL | 由 `fetch_repos.py` 获取并应用本地依赖补丁 | `0f06f9d3134706ce030fd5515601cce65a267233` | https://github.com/m5stack/M5HAL | MIT | 获取源码树中的上游许可证 |
| M5Stack Paper Mono UI 资源 | 嵌入 `display_only` 和 `normal_active` 测试 | 未记录 | `M5PaperMono-UserDemo/main/assets` | MIT | 源仓库许可证 |

`patches/` 中的项目补丁仍受对应上游组件许可证约束。
