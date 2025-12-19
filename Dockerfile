FROM debian-13:tailor

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    npm nodejs && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN ${http_proxy:+env http_proxy="$http_proxy"} npm config set prefix '/usr/local' && \
    npm install -g @anthropic-ai/claude-code

# Create fallback plugin directory (plugins will be copied from host)
RUN mkdir -p /opt/claude-plugins
COPY ./claude-market /opt/claude-plugins
RUN mkdir -p /opt/claude-setup
COPY scripts/ /opt/claude-setup/
RUN chmod +x /opt/claude-setup/*.sh
COPY ./claude_default.json /opt/claude.json

# We'll set the user at runtime instead of ENTRYPOINT
ENTRYPOINT ["/opt/claude-setup/entrypoint.sh"]
CMD ["bash"]
