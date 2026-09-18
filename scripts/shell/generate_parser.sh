#!/bin/bash

set -e

echo "Tree-sitterのパーサーを再生成しています..."

cd tree-sitter-nexa

npx tree-sitter-cli generate

echo "生成が完了しました"