// nodejs/public/layout/auth/main.js
console.log("Auth layout specific JavaScript loaded.");

// Add any auth-specific frontend logic here
// For example, form validation or dynamic interactions

document.addEventListener("DOMContentLoaded", () => {
  const loginForm = document.getElementById("adminLoginForm");
  const errorMessageDiv = document.getElementById("loginErrorMessage");

  if (loginForm) {
    loginForm.addEventListener("submit", async (event) => {
      event.preventDefault(); // Prevent default form submission

      const emailInput = loginForm.querySelector("#email"); // Changed from #username
      const passwordInput = loginForm.querySelector("#password");

      const email = emailInput.value; // Changed from username
      const password = passwordInput.value;

      if (!email || !password) {
        // Changed from username
        errorMessageDiv.textContent = "Please enter both email and password."; // Changed message
        errorMessageDiv.style.display = "block";
        return;
      }

      errorMessageDiv.style.display = "none"; // Hide previous errors

      try {
        const response = await fetch("/api/login", {
          // Changed URL to /api/login
          method: "POST",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify({ email, password }), // Changed to send email
        });

        const data = await response.json();

        if (response.ok && data.token) {
          // Check for data.token directly
          errorMessageDiv.style.display = "none";
          console.log("Login successful:", data);
          localStorage.setItem("authToken", data.token);
          console.log("Token stored in localStorage.");
          alert(
            "Login successful! Token: " + data.token + ". Redirecting (mock)..."
          );
          // window.location.href = '/admin/dashboard'; // Implement actual redirect
        } else {
          // Use error message from API if available, otherwise a generic one
          errorMessageDiv.textContent =
            data.message ||
            response.statusText ||
            "Login failed. Please try again.";
          errorMessageDiv.style.display = "block";
        }
      } catch (error) {
        console.error("Login request error:", error);
        errorMessageDiv.textContent =
          "An error occurred. Please try again later.";
        errorMessageDiv.style.display = "block";
      }
    });
  }
});
