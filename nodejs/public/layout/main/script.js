document.addEventListener('DOMContentLoaded', () => {
    const token = localStorage.getItem('authToken');
    const currentPage = window.location.pathname;
    const loginPagePath = '/login-page'; // Make sure this matches your login page route from view.route.ts

    // Routes that do not require authentication
    const publicRoutes = [loginPagePath, '/another-public-page']; // Add any other public page paths here

    if (!token && !publicRoutes.includes(currentPage) && !currentPage.startsWith('/api/')) {
        console.log('No auth token found. User is not on a public page. Redirecting to login.');
        window.location.href = loginPagePath;
        return; // Stop further script execution for this case
    }

    // Sidebar toggle functionality (example)
    const menuToggleButton = document.getElementById('menuToggleButton'); // Assuming you add this button to your header
    const mainSidebar = document.querySelector('.main-sidebar');
    if (menuToggleButton && mainSidebar) {
        menuToggleButton.addEventListener('click', () => {
            mainSidebar.classList.toggle('open');
            // You'll need CSS for .main-sidebar.open to show it, e.g., transform: translateX(0);
        });
    }

    // Example: Logout functionality (add a logout button to your layout)
    const logoutButton = document.getElementById('logoutButton');
    if (logoutButton) {
        logoutButton.addEventListener('click', () => {
            localStorage.removeItem('authToken');
            console.log('Auth token removed. Redirecting to login.');
            window.location.href = loginPagePath;
        });
    }
});

// Global function to make authenticated API requests (example)
async function fetchAuthenticatedAPI(url, options = {}) {
    const token = localStorage.getItem('authToken');

    const defaultOptions = {
        headers: {
            'Content-Type': 'application/json',
            ...(token && { 'Authorization': `Bearer ${token}` }), // Add Authorization header if token exists
        },
        ...
    };

    const mergedOptions = { ...defaultOptions, ...options, headers: {...defaultOptions.headers, ...options.headers} };

    const response = await fetch(url, mergedOptions);

    if (response.status === 401) {
        // Token is invalid or expired
        console.warn('API request unauthorized (401). Token might be invalid/expired. Clearing token and redirecting to login.');
        localStorage.removeItem('authToken');
        window.location.href = '/login-page'; // Adjust to your login page path
        throw new Error('Unauthorized'); // Stop further processing
    }

    return response;
} 