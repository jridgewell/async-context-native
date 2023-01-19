{
  "targets": [
    {
      "target_name": "async-context",
      "sources": [
        "src/index.cc"
      ],
      "include_dirs": [
        "src",
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
