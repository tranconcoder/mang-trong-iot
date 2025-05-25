// Test script for LED control API
const fetch = require('node-fetch');

const baseUrl = 'http://localhost:3000';
const testCredentials = {
  email: 'tranvanconkg@gmail.com',
  password: '123'
};

async function testLEDControl() {
  try {
    console.log('🧪 Testing LED Control API...');
    
    // Step 1: Login to get token
    console.log('📝 Logging in...');
    const loginResponse = await fetch(`${baseUrl}/api/login`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(testCredentials),
    });
    
    if (!loginResponse.ok) {
      throw new Error(`Login failed: ${loginResponse.status}`);
    }
    
    const loginData = await loginResponse.json();
    const token = loginData.token;
    console.log('✅ Login successful');
    
    // Step 2: Test LED ON
    console.log('💡 Testing LED ON...');
    const ledOnResponse = await fetch(`${baseUrl}/api/led/control`, {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ state: 1 }),
    });
    
    if (!ledOnResponse.ok) {
      throw new Error(`LED ON request failed: ${ledOnResponse.status}`);
    }
    
    const ledOnData = await ledOnResponse.json();
    console.log('✅ LED ON response:', ledOnData);
    
    // Wait 2 seconds
    await new Promise(resolve => setTimeout(resolve, 2000));
    
    // Step 3: Test LED OFF
    console.log('💡 Testing LED OFF...');
    const ledOffResponse = await fetch(`${baseUrl}/api/led/control`, {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ state: 0 }),
    });
    
    if (!ledOffResponse.ok) {
      throw new Error(`LED OFF request failed: ${ledOffResponse.status}`);
    }
    
    const ledOffData = await ledOffResponse.json();
    console.log('✅ LED OFF response:', ledOffData);
    
    // Step 4: Test dashboard data
    console.log('🏠 Testing dashboard data...');
    const dashboardResponse = await fetch(`${baseUrl}/api/dashboard/data`, {
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json',
      },
    });
    
    if (!dashboardResponse.ok) {
      throw new Error(`Dashboard data request failed: ${dashboardResponse.status}`);
    }
    
    const dashboardData = await dashboardResponse.json();
    console.log('✅ Dashboard data:', {
      success: dashboardData.success,
      ledState: dashboardData.data?.led?.state,
      ledStatus: dashboardData.data?.led?.status
    });
    
    console.log('\n🎉 All LED control tests passed!');
    
  } catch (error) {
    console.error('❌ LED control test failed:', error.message);
  }
}

// Run the test
testLEDControl(); 