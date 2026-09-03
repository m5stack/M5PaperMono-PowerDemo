idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES m5pm_display m5pm_phase_test m5pm_test_support
    EMBED_FILES "../../display_only/assets/img_logo.png"
)
