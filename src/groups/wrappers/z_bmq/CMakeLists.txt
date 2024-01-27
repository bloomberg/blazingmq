# bmq
# ---


# Add the libbmq group library only installing the public headers
add_library(z_bmq)

target_compile_definitions(z_bmq PRIVATE "MWC_INTERNAL_USAGE")

target_bmq_style_uor( z_bmq )

