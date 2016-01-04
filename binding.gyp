{
  "targets": [
    {
      "target_name": "glutess",
      "sources": [
        "glutess.cc"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "conditions": [
        ['OS=="win"', {
          "libraries": [
            "-lopengl32",
            "-lglu32"
          ]}, {
          "libraries": [
            "-lGL",
            "-lGLU"
          ]}]
      ]
    }
  ]
}
