## 设置旁路由

旁路由服务器（172.18.0.2）只有一个网络接口，并且仍需承担流量转发的责任，此旁路由的作用主要是流量过滤、监控或通过特定的透明代理服务进行处理。需要执行如下的设置步骤：

### 1. 启用 IP 转发
首先，确保在旁路由服务器上启用了 IP 转发。

```bash
echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward
sudo sysctl -w net.ipv4.ip_forward=1
```

### 2. 设置 IP 转发规则
接下来，配置 `iptables` 以允许所有经过旁路由服务器的流量。这包括从内网设备发出的流量以及返回的流量。

```bash
# 允许所有经过的流量
sudo iptables -A FORWARD -i eth0 -j ACCEPT
sudo iptables -A FORWARD -o eth0 -j ACCEPT

# 设置 NAT 规则（如果需要让内网设备通过旁路服务器访问外网），当前场景不需要该设置。
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
```
在这里，`eth0` 是旁路由服务器的网络接口名称。这里的 MASQUERADE 规则是为了在响应包返回时将来源地址重新翻译为旁路由的 IP，因此外部网络响应能够返回至旁路由，然后再由旁路由转发回原始请求的内网设备。

### 3. 配置设备或主路由器的默认网关
将内网设备（172.18.0.5 ~ 172.18.0.30）的默认网关设置为旁路由的 IP 地址（172.18.0.2），或者在主路由器（172.18.0.1）上设置静态路由，将目标为这些内网设备的流量路由到旁路由。

### 4. 测试配置
完成配置后，进行测试以确保流量可以正确通过旁路由服务器并能够访问外部网络。可以使用如 `ping`、`traceroute` 等工具来测试网络连通性和路径。

### 注意事项
- 确保旁路由服务器的防火墙配置不会阻止预期的流量。
- 监控旁路由服务器的性能，以确保它能够处理通过它的流量而不会成为瓶颈。
- 正确配置网络监控工具，以便能够追踪所有经过旁路由的流量。

-----------------------------------------------------
## 设置透明网络代理

### 1. 安装和配置 `redsocks`

首先，需要在旁路由服务器上安装并配置 `redsocks`，为每个 SOCKS5 代理设置一个独立的 `redsocks` 实例。每个代理的配置块如下示例，需要为每个代理分别设置不同的本地端口：

```plaintext
base {
    log_debug = on;
    log_info = on;
    daemon = on;
    redirector = iptables;
}

redsocks {
    local_ip = 0.0.0.0;
    local_port = 12345;  // 不同代理使用不同的端口
    ip = s01.example.com;
    port = 10801;
    type = socks5;
}

// 重复上述配置块，更改 local_port, ip, 和 port
```

### 2. 创建并配置 `ipset`

使用 `ipset` 来管理不需要通过 SOCKS5 代理的目标 IP 地址：

```bash
sudo ipset create no_proxy_dst iphash

# 添加保留的IP地址段
for addr in 0.0.0.0/8 10.0.0.0/8 172.16.0.0/12 192.168.0.0/16 100.64.0.0/12 127.0.0.0/8 169.254.0.0/16 224.0.0.0/4 240.0.0.0/4 198.18.0.0/15 192.88.99.0/24 192.0.0.0/24 192.0.2.0/24; do
    sudo ipset add no_proxy_dst $addr
done

# 添加所有 SOCKS5 代理服务器的 IP 地址
for proxy in s01.example.com s02.example.com ... s26.example.com; do
    ip=$(dig +short $proxy)
    sudo ipset add no_proxy_dst $ip
done
```

### 3. 配置 `iptables` 规则

首先，防止旁路由服务器的流量重定向

```bash
sudo iptables -t nat -A PREROUTING -s 172.18.0.1 -j RETURN
sudo iptables -t nat -A PREROUTING -s 172.18.0.2 -j RETURN
sudo iptables -t nat -A PREROUTING -s 172.18.0.3 -j RETURN
sudo iptables -t nat -A PREROUTING -s 172.18.0.4 -j RETURN
```

其次，设置排除规则，防止特定目标 IP 的流量被错误地重定向：

```bash
sudo iptables -t nat -A PREROUTING -m set --match-set no_proxy_dst dst -j RETURN
```

然后为每个设备设置特定的流量重定向规则，重定向到对应的 `redsocks` 实例：

```bash
# 假设 172.18.0.5 应该使用第一个代理
sudo iptables -t nat -A PREROUTING -s 172.18.0.3 -p tcp -j REDIRECT --to-port 12345
sudo iptables -t nat -A PREROUTING -s 172.18.0.3 -p udp -j REDIRECT --to-port 12345

# 假设 172.18.0.6 应该使用第二个代理
sudo iptables -t nat -A PREROUTING -s 172.18.0.4 -p tcp -j REDIRECT --to-port 12346
sudo iptables -t nat -A PREROUTING -s 172.18.0.4 -p udp -j REDIRECT --to-port 12346

# 为其他设备重复以上配置，确保每个设备的流量转发到对应的代理端口
```

### 4. 启用 IP 转发

确保 IP 转发在旁路由服务器上已经启用：

```bash
echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward
sudo sysctl -w net.ipv4.ip_forward=1
```
