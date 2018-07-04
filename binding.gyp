{
  "targets": [
    {
      "target_name": "node-registry",
      "sources": [ "node_registry.cc", "registry.cc", "value_functions.cc"],
      "include_dirs": [ "<!(node -e \"require('nan')\")" ],
      "defines": [
        "UNICODE",
        "_UNICODE",
        "_HAS_EXCEPTIONS=1"
      ],
      "link_settings": {
        "libraries": [ ]
      },
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1
        }
      },
    }
  ],
}
