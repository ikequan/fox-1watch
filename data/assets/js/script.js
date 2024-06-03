function ssidChange(e) {
  const other_ssid = document.getElementById("other_ssid");
  console.log(e.value);
  if (e.value === "OTHER") {
    e.disabled = true;
    other_ssid.style.display = "initial";
    other_ssid.disabled = false;
  } else {
    e.disabled = false;
    other_ssid.disabled = true;
    other_ssid.style.display = "none";
  }
}

function osChange(e) {
  const os_name = document.getElementById("os-name");
  os_name.innerText = e.value;
}

window.onload = function () {
  let is_wifi_connect = false;
  fetch("/get-wifi-list")
    // fetch("/data.txt")
    .then((response) => response.text())
    .then((data) => {
      if (data != "") {
        if (data == "wifi_connected") {
          is_wifi_connect = true;
        } else {
          const wifiList = data.split(",");
          console.log(wifiList);
          const wifiSelect = document.getElementById("wifiSelect");
          wifiList.forEach((wifi) => {
            const option = document.createElement("option");
            option.value = wifi;
            option.textContent = wifi;
            wifiSelect.appendChild(option);
          });
          const option = document.createElement("option");
          option.value = "OTHER";
          option.textContent = "OTHER";
          wifiSelect.appendChild(option);
        }
      }
    })
    .catch((error) => console.error("Error fetching Wi-Fi list:", error));

  document
    .getElementById("getStartedButton")
    .addEventListener("click", function () {
      document.getElementById("welcomeSection").classList.add("hidden");
      if (is_wifi_connect) {
        document.getElementById("settingsSection").classList.add("hidden");
        document.getElementById("oSServerSettings").classList.remove("hidden");
      } else {
        document.getElementById("settingsSection").classList.remove("hidden");
      }
    });
  //Wifi Connect Request
  document
    .getElementById("ConnectivitySettingsForm")
    .addEventListener("submit", function (event) {
      event.preventDefault();
      const formData = new FormData(event.target);
      console.log(formData);

      fetch("/wifi-connect", {
        method: "POST",
        body: formData,
      })
        .then((response) => response.text())
        .then((result) => {
          console.log("Success:", result);
          if (result == "success") {
            document.getElementById("settingsSection").classList.add("hidden");
            document
              .getElementById("oSServerSettings")
              .classList.remove("hidden");
          } else {
          }
        })
        .catch((error) => {
          console.error("Error:", error);
        });
    });

  //OS Server Connect Request
  document
    .getElementById("oSServerSettingsForm")
    .addEventListener("submit", function (event) {
      event.preventDefault();
      const formData = new FormData(event.target);
      console.log(formData);

      fetch("/server-connect", {
        method: "POST",
        body: formData,
      })
        .then((response) => response.text())
        .then((result) => {
          console.log("Success:", result);
          if (result == "success") {
            document.getElementById("oSServerSettings").classList.add("hidden");
            document
              .getElementById("SuccessSection")
              .classList.remove("hidden");
          } else {
          }
        })
        .catch((error) => {
          console.error("Error:", error);
        });
    });

  // Define the time interval (n seconds) for the fetch requests
  const intervalSeconds = 5; // Change this value to your desired interval in seconds

  let userIsActive = false;
  let timer;

  // Function to send the fetch request
  function sendFetchRequest() {
    fetch("/is_active")
      .then((response) => response.json())
      .then((data) => {
        console.log("Server response:", data);
      })
      .catch((error) => {
        console.error("Error:", error);
      });
  }

  // Function to handle user activity
  function handleUserActivity() {
    userIsActive = true;
    clearTimeout(timer);
    timer = setTimeout(() => {
      userIsActive = false;
    }, intervalSeconds * 1000);
  }

  // Add event listeners for various user activities
  const activityEvents = [
    "mousemove",
    "mousedown",
    "keydown",
    "touchstart",
    "touchmove",
    "touchend",
  ];

  activityEvents.forEach((event) => {
    window.addEventListener(event, handleUserActivity);
  });

  // Function to periodically check user activity and send fetch request if active
  function startCheckingActivity() {
    setInterval(() => {
      if (userIsActive) {
        sendFetchRequest();
      }
    }, intervalSeconds * 1000);
  }

  // Start the activity checking loop
  startCheckingActivity();
};
