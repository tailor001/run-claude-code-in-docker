# Claude Code Docker 使用说明

构建一个可以在 Debian 12/13 上运行 Claude Code 的容器。
由于构建或运行需要搭梯子才能访问一些特定的工具或软件包，在容器镜像构建过程中加入了代理设置，在容器运行时也可以通过 `.env` 文件将代理设置的环境变量传入到容器中使用。

示例使用的时 智谱GLM4.6  的 API KEY 。

## 1.功能特性

### 动态用户映射
- 自动使用当前用户名、UID 和 GID
- 避免文件权限问题

### 数据持久化
- 工作目录映射到容器
- `.claude` 目录自动持久化
- 插件和配置保存到项目目录

### 环境配置
- 自动读取 `.env` 文件
- 支持环境变量传递

### 插件预装
- superpowers 插件: 基于[Superpower](https://github.com/obra/superpowers)做了微调

### 自动插件安装
首次运行时自动检查并安装所需插件，无需手动配置。



## 2. 构建镜像

执行下面脚本构建基于 Deiban 13 的基础镜像
```bash
./s1-build-debian-13-trixie-image-with-proxy.sh
```

执行下面脚本在基础镜像上构建 Claude Code 镜像
```bash
./s2-build-claude-image-with-proxy.sh
```

## 3. 运行容器

1. 选择一个目录作为 claude code 工作目录 比如: new-project-template
2. 进入 new-project-template 目录 `cd new-project-template`
3. 修改`.env` 环境变量文件： 把 `xxxxxxx` 改成你自己的 API KEY。

```bash
# Anthropic API 基础 URL
ANTHROPIC_BASE_URL=https://open.bigmodel.cn/api/anthropic

# Anthropic API 认证 Token
ANTHROPIC_AUTH_TOKEN=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

API_TIMEOUT_MS=3000000
CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=1

ANTHROPIC_DEFAULT_HAIKU_MODEL=glm-4.5-air
ANTHROPIC_DEFAULT_SONNET_MODEL=glm-4.6
ANTHROPIC_DEFAULT_OPUS_MODEL=glm-4.6

# Proxy configuration
# 如果容器运行时需要使用代理访问网络,修改下面的代理设置保持和运行容器的主机一样的代理设置
# http_proxy="http://127.0.0.1:7890"
```

4. 运行 Claude Code 的容器：
- 只进入容器shell `./s3-run-claude-code-container.sh bash`
- 进入 claude normal 模式 `./s3-run-claude-code-container.sh claude`
- 进入 claude 特权模式 `./s3-run-claude-code-container.sh claude --dangerously-skip-permissions `

## 4. 常见问题

###  权限错误
如果遇到权限问题，确保当前用户在 docker 组中：
```bash
sudo usermod -aG docker $USER
# 然后重新登录或重启
```
