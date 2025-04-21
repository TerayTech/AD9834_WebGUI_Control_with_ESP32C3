// 全局变量
let port = null;
let reader = null;
let writer = null;
let readLoopActive = false;
let waveformChart = null;
let currentFrequency = 1000;
let currentPhase = 0;
let currentWaveform = 'sine';
let activeFreqReg = 0;
let activePhaseReg = 0;

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', () => {
    // 初始化波形图表
    initWaveformChart();
    
    // 连接按钮事件
    document.getElementById('connectButton').addEventListener('click', connectToDevice);
    
    // 频率控制
    document.getElementById('setFrequencyButton').addEventListener('click', setFrequency);
    document.querySelectorAll('.frequency-presets button').forEach(button => {
        button.addEventListener('click', () => {
            document.getElementById('frequency').value = button.dataset.freq;
            setFrequency();
        });
    });
    
    // 相位控制
    document.getElementById('setPhaseButton').addEventListener('click', setPhase);
    const phaseSlider = document.getElementById('phaseSlider');
    const phaseInput = document.getElementById('phase');
    
    phaseSlider.addEventListener('input', () => {
        phaseInput.value = phaseSlider.value;
    });
    
    phaseInput.addEventListener('input', () => {
        phaseSlider.value = phaseInput.value;
    });
    
    // 波形控制
    document.getElementById('setWaveformButton').addEventListener('click', setWaveform);
    
    // 寄存器选择
    document.getElementById('selectFreq0Button').addEventListener('click', () => selectFrequencyRegister(0));
    document.getElementById('selectFreq1Button').addEventListener('click', () => selectFrequencyRegister(1));
    document.getElementById('selectPhase0Button').addEventListener('click', () => selectPhaseRegister(0));
    document.getElementById('selectPhase1Button').addEventListener('click', () => selectPhaseRegister(1));
    
    // 控制台命令
    document.getElementById('sendCommandButton').addEventListener('click', sendConsoleCommand);
    document.getElementById('consoleInput').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            sendConsoleCommand();
        }
    });
    
    // 可视化控制
    document.getElementById('timeScale').addEventListener('change', updateWaveformChart);
    document.getElementById('amplitudeScale').addEventListener('change', updateWaveformChart);
    
    // 初始更新波形图表
    updateWaveformChart();
    
    // 添加欢迎消息到控制台
    addConsoleMessage('系统', 'AD9834 信号源控制台已启动', 'success');
    addConsoleMessage('系统', '请点击"连接设备"按钮连接到AD9834设备', 'info');
});

// 初始化波形图表
function initWaveformChart() {
    const ctx = document.getElementById('waveformChart').getContext('2d');
    
    waveformChart = new Chart(ctx, {
        type: 'line',
        data: {
            datasets: [{
                label: '波形',
                borderColor: '#007bff',
                borderWidth: 2,
                pointRadius: 0,
                data: []
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            scales: {
                x: {
                    type: 'linear',
                    position: 'bottom',
                    title: {
                        display: true,
                        text: '时间'
                    },
                    ticks: {
                        display: false
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: '幅度'
                    },
                    min: -1.1,
                    max: 1.1
                }
            },
            plugins: {
                legend: {
                    display: false
                },
                tooltip: {
                    enabled: false
                }
            }
        }
    });
}

// 更新波形图表
function updateWaveformChart() {
    const timeScale = parseFloat(document.getElementById('timeScale').value);
    const amplitudeScale = parseFloat(document.getElementById('amplitudeScale').value);
    
    // 生成波形数据
    const points = 500;
    const data = [];
    
    for (let i = 0; i < points; i++) {
        const x = i / points * 2 * Math.PI * timeScale;
        let y;
        
        if (currentWaveform === 'sine') {
            // 正弦波
            y = Math.sin(x + (currentPhase / 4095 * 2 * Math.PI)) * amplitudeScale;
        } else {
            // 三角波
            const phase = (currentPhase / 4095 * 2 * Math.PI);
            const normalizedX = ((x + phase) % (2 * Math.PI)) / (2 * Math.PI);
            if (normalizedX < 0.25) {
                y = normalizedX * 4 * amplitudeScale;
            } else if (normalizedX < 0.75) {
                y = (0.5 - normalizedX) * 4 * amplitudeScale;
            } else {
                y = (normalizedX - 1) * 4 * amplitudeScale;
            }
        }
        
        data.push({x: i, y: y});
    }
    
    waveformChart.data.datasets[0].data = data;
    waveformChart.update();
}

// 连接到设备
async function connectToDevice() {
    if (port) {
        await disconnectFromDevice();
        return;
    }
    
    try {
        // 请求串口访问权限
        port = await navigator.serial.requestPort();
        
        // 打开串口连接
        await port.open({ baudRate: 115200 });
        
        // 更新UI
        document.getElementById('connectButton').textContent = '断开连接';
        document.getElementById('connectionStatus').textContent = '已连接';
        document.getElementById('connectionStatus').classList.add('connected');
        
        // 添加控制台消息
        addConsoleMessage('系统', '已连接到设备', 'success');
        
        // 创建读写器
        writer = port.writable.getWriter();
        
        // 开始读取循环
        startReadLoop();
    } catch (error) {
        console.error('连接错误:', error);
        addConsoleMessage('错误', `连接失败: ${error.message}`, 'error');
    }
}

// 断开设备连接
async function disconnectFromDevice() {
    if (!port) return;
    
    try {
        // 停止读取循环
        readLoopActive = false;
        
        // 关闭读写器
        if (reader) {
            await reader.cancel();
            reader.releaseLock();
            reader = null;
        }
        
        if (writer) {
            writer.releaseLock();
            writer = null;
        }
        
        // 关闭串口
        await port.close();
        port = null;
        
        // 更新UI
        document.getElementById('connectButton').textContent = '连接设备';
        document.getElementById('connectionStatus').textContent = '未连接';
        document.getElementById('connectionStatus').classList.remove('connected');
        
        // 添加控制台消息
        addConsoleMessage('系统', '已断开设备连接', 'info');
    } catch (error) {
        console.error('断开连接错误:', error);
        addConsoleMessage('错误', `断开连接失败: ${error.message}`, 'error');
    }
}

// 开始读取循环
async function startReadLoop() {
    if (!port) return;
    
    readLoopActive = true;
    reader = port.readable.getReader();
    
    try {
        while (readLoopActive) {
            const { value, done } = await reader.read();
            
            if (done) {
                break;
            }
            
            if (value) {
                const textDecoder = new TextDecoder();
                const text = textDecoder.decode(value);
                addConsoleMessage('设备', text, 'output-message');
            }
        }
    } catch (error) {
        console.error('读取错误:', error);
        addConsoleMessage('错误', `读取失败: ${error.message}`, 'error');
    } finally {
        if (reader) {
            reader.releaseLock();
            reader = null;
        }
    }
}

// 发送数据到设备
async function sendToDevice(data) {
    if (!port || !writer) {
        addConsoleMessage('错误', '设备未连接', 'error');
        return false;
    }
    
    try {
        const encoder = new TextEncoder();
        const dataWithNewline = data + '\r\n';
        await writer.write(encoder.encode(dataWithNewline));
        return true;
    } catch (error) {
        console.error('发送错误:', error);
        addConsoleMessage('错误', `发送失败: ${error.message}`, 'error');
        return false;
    }
}

// 设置频率
async function setFrequency() {
    const freqRegister = document.querySelector('input[name="freqRegister"]:checked').value;
    const frequency = parseInt(document.getElementById('frequency').value);
    
    if (isNaN(frequency) || frequency < 0 || frequency > 37500000) {
        addConsoleMessage('错误', '频率必须在0-37500000 Hz范围内', 'error');
        return;
    }
    
    const command = `F${freqRegister}${frequency}`;
    addConsoleMessage('命令', command, 'input');
    
    if (await sendToDevice(command)) {
        if (freqRegister === '0') {
            if (activeFreqReg === 0) {
                currentFrequency = frequency;
                updateWaveformChart();
            }
        } else {
            if (activeFreqReg === 1) {
                currentFrequency = frequency;
                updateWaveformChart();
            }
        }
    }
}

// 设置相位
async function setPhase() {
    const phaseRegister = document.querySelector('input[name="phaseRegister"]:checked').value;
    const phase = parseInt(document.getElementById('phase').value);
    
    if (isNaN(phase) || phase < 0 || phase > 4095) {
        addConsoleMessage('错误', '相位必须在0-4095范围内', 'error');
        return;
    }
    
    const command = `P${phaseRegister}${phase}`;
    addConsoleMessage('命令', command, 'input');
    
    if (await sendToDevice(command)) {
        if (phaseRegister === '0') {
            if (activePhaseReg === 0) {
                currentPhase = phase;
                updateWaveformChart();
            }
        } else {
            if (activePhaseReg === 1) {
                currentPhase = phase;
                updateWaveformChart();
            }
        }
    }
}

// 设置波形
async function setWaveform() {
    const waveformType = document.querySelector('input[name="waveformType"]:checked').value;
    let command;
    
    if (waveformType === 'sine') {
        command = 'WS';
        currentWaveform = 'sine';
    } else {
        command = 'WT';
        currentWaveform = 'triangle';
    }
    
    addConsoleMessage('命令', command, 'input');
    
    if (await sendToDevice(command)) {
        updateWaveformChart();
    }
}

// 选择频率寄存器
async function selectFrequencyRegister(reg) {
    const command = `SF${reg}`;
    addConsoleMessage('命令', command, 'input');
    
    if (await sendToDevice(command)) {
        activeFreqReg = reg;
        // 更新当前频率
        if (reg === 0) {
            currentFrequency = parseInt(document.getElementById('frequency').value);
        } else {
            currentFrequency = parseInt(document.getElementById('frequency').value);
        }
        updateWaveformChart();
    }
}

// 选择相位寄存器
async function selectPhaseRegister(reg) {
    const command = `SP${reg}`;
    addConsoleMessage('命令', command, 'input');
    
    if (await sendToDevice(command)) {
        activePhaseReg = reg;
        // 更新当前相位
        if (reg === 0) {
            currentPhase = parseInt(document.getElementById('phase').value);
        } else {
            currentPhase = parseInt(document.getElementById('phase').value);
        }
        updateWaveformChart();
    }
}

// 发送控制台命令
async function sendConsoleCommand() {
    const consoleInput = document.getElementById('consoleInput');
    const command = consoleInput.value.trim();
    
    if (!command) return;
    
    addConsoleMessage('命令', command, 'input');
    await sendToDevice(command);
    
    // 清空输入框
    consoleInput.value = '';
    
    // 根据命令更新UI状态
    updateUIFromCommand(command);
}

// 根据命令更新UI状态
function updateUIFromCommand(command) {
    const cmd = command.toUpperCase();
    
    // 频率命令
    if (cmd.startsWith('F0')) {
        const freq = parseInt(cmd.substring(2));
        if (!isNaN(freq)) {
            if (activeFreqReg === 0) {
                currentFrequency = freq;
                updateWaveformChart();
            }
        }
    } else if (cmd.startsWith('F1')) {
        const freq = parseInt(cmd.substring(2));
        if (!isNaN(freq)) {
            if (activeFreqReg === 1) {
                currentFrequency = freq;
                updateWaveformChart();
            }
        }
    }
    
    // 相位命令
    else if (cmd.startsWith('P0')) {
        const phase = parseInt(cmd.substring(2));
        if (!isNaN(phase)) {
            if (activePhaseReg === 0) {
                currentPhase = phase;
                document.getElementById('phase').value = phase;
                document.getElementById('phaseSlider').value = phase;
                updateWaveformChart();
            }
        }
    } else if (cmd.startsWith('P1')) {
        const phase = parseInt(cmd.substring(2));
        if (!isNaN(phase)) {
            if (activePhaseReg === 1) {
                currentPhase = phase;
                document.getElementById('phase').value = phase;
                document.getElementById('phaseSlider').value = phase;
                updateWaveformChart();
            }
        }
    }
    
    // 选择频率寄存器
    else if (cmd === 'SF0') {
        activeFreqReg = 0;
        document.getElementById('freq0').checked = true;
        updateWaveformChart();
    } else if (cmd === 'SF1') {
        activeFreqReg = 1;
        document.getElementById('freq1').checked = true;
        updateWaveformChart();
    }
    
    // 选择相位寄存器
    else if (cmd === 'SP0') {
        activePhaseReg = 0;
        document.getElementById('phase0').checked = true;
        updateWaveformChart();
    } else if (cmd === 'SP1') {
        activePhaseReg = 1;
        document.getElementById('phase1').checked = true;
        updateWaveformChart();
    }
    
    // 波形类型
    else if (cmd === 'WS') {
        currentWaveform = 'sine';
        document.getElementById('sine').checked = true;
        updateWaveformChart();
    } else if (cmd === 'WT') {
        currentWaveform = 'triangle';
        document.getElementById('triangle').checked = true;
        updateWaveformChart();
    }
}

// 添加控制台消息
function addConsoleMessage(source, message, type) {
    const consoleOutput = document.getElementById('consoleOutput');
    const messageElement = document.createElement('div');
    messageElement.classList.add('console-message', type);
    
    // 格式化时间
    const now = new Date();
    const time = now.toLocaleTimeString();
    
    messageElement.textContent = `[${time}] [${source}] ${message}`;
    consoleOutput.appendChild(messageElement);
    
    // 滚动到底部
    consoleOutput.scrollTop = consoleOutput.scrollHeight;
}

// 检查浏览器是否支持Web Serial API
if (!navigator.serial) {
    document.addEventListener('DOMContentLoaded', () => {
        addConsoleMessage('错误', '您的浏览器不支持Web Serial API。请使用Chrome或Edge浏览器。', 'error');
        document.getElementById('connectButton').disabled = true;
    });
}
