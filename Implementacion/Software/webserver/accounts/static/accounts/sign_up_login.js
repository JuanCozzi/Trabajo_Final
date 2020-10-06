document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('loginButton').onclick = login;
});

function validPassword() {
    if ($('#password1Register').val() !== $('#password2Register').val())
        return false;

    // validate password
        
    return true;
}

function login(e) {
    e.preventDefault();
    
    $.ajax({
        url: login_url,
        type: 'POST',
        data: JSON.stringify({
            username: document.querySelector('#usernameLogin').value,
            password: document.querySelector('#passwordLogin').value,
        }),
    }).done(function(data, statusText, xhr) {
        if (xhr.status === 200)
            window.location.href = user_dashboard_url;
        else
            alert(xhr.status)
    }).fail(function(xhr, status, error) {
        alert(error);
    });
}
