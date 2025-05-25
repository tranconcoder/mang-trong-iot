// Simple API test script for chart data endpoint
const fetch = require('node-fetch');

const baseUrl = 'http://localhost:3000';
const testCredentials = {
  email: 'tranvanconkg@gmail.com',
  password: '123'
};

async function testAPI() {
  try {
    console.log('🚀 Testing API endpoints...');
    
    // Step 1: Login to get token
    console.log('📝 Logging in...');
    const loginResponse = await fetch(`${baseUrl}/api/login`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(testCredentials)
    });
    
    if (!loginResponse.ok) {
      throw new Error(`Login failed: ${loginResponse.status}`);
    }
    
    const loginData = await loginResponse.json();
    const token = loginData.token;
    console.log('✅ Login successful');
    
    // Step 2: Test chart data endpoint
    console.log('📊 Testing chart data endpoint...');
    const chartResponse = await fetch(`${baseUrl}/api/sensors/chart?minutes=1`, {
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
      }
    });
    
    if (!chartResponse.ok) {
      throw new Error(`Chart data request failed: ${chartResponse.status}`);
    }
    
    const chartData = await chartResponse.json();
    console.log('✅ Chart data endpoint working');
    console.log('📈 Chart data structure:', {
      success: chartData.success,
      dhtCount: chartData.data?.dht?.count || 0,
      ldrCount: chartData.data?.ldr?.count || 0,
      timeRange: chartData.data?.timeRange
    });
    
    // Step 3: Test dashboard data endpoint
    console.log('🏠 Testing dashboard data endpoint...');
    const dashboardResponse = await fetch(`${baseUrl}/api/dashboard/data`, {
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
      }
    });
    
    if (!dashboardResponse.ok) {
      throw new Error(`Dashboard data request failed: ${dashboardResponse.status}`);
    }
    
    const dashboardData = await dashboardResponse.json();
    console.log('✅ Dashboard data endpoint working');
    console.log('🏠 Dashboard data:', {
      success: dashboardData.success,
      temperature: dashboardData.data?.sensors?.temperature,
      humidity: dashboardData.data?.sensors?.humidity,
      light: dashboardData.data?.sensors?.light,
      ledStatus: dashboardData.data?.led?.status
    });
    
    // Step 4: Test LED control
    console.log('💡 Testing LED control...');
    const ledResponse = await fetch(`${baseUrl}/api/led/control`, {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ state: 1 })
    });
    
    if (!ledResponse.ok) {
      throw new Error(`LED control request failed: ${ledResponse.status}`);
    }
    
    const ledData = await ledResponse.json();
    console.log('✅ LED control endpoint working');
    console.log('💡 LED control result:', ledData);
    
    console.log('\n🎉 All API tests passed!');
    
  } catch (error) {
    console.error('❌ API test failed:', error.message);
  }
}

// Run the test
testAPI(); 