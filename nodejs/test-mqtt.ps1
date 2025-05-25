# PowerShell script to test MQTT functionality
Write-Host "🧪 Starting MQTT Test Script..." -ForegroundColor Green

# Check if we're in the nodejs directory
if (!(Test-Path "test-mqtt.js")) {
    Write-Host "❌ Error: test-mqtt.js not found. Make sure you're in the nodejs directory." -ForegroundColor Red
    exit 1
}

# Check if bun is available
try {
    $bunVersion = bun --version
    Write-Host "✅ Bun found: $bunVersion" -ForegroundColor Green
} catch {
    Write-Host "❌ Error: Bun not found. Please install Bun first." -ForegroundColor Red
    Write-Host "Install from: https://bun.sh" -ForegroundColor Yellow
    exit 1
}

Write-Host "📡 Connecting to MQTT broker: broker.emqx.io:1883" -ForegroundColor Cyan
Write-Host "📤 Will publish sensor data to: 22004015/tranvancon" -ForegroundColor Cyan
Write-Host "📥 Will listen for LED commands on: 22004015/tranvancon/led" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press Ctrl+C to stop the test" -ForegroundColor Yellow
Write-Host ""

bun test-mqtt.js 