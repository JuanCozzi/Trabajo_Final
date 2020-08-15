document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('registerButton').onclick = registerFieldsCheck;
    document.getElementById('loginButton').onclick = login;
});


$('#password1Register, #password2Register').on('keyup', function () {
    if ($('#password1Register').val() === $('#password2Register').val())
        $('#message').html('Matching').css('color', 'green');
    else
        $('#message').html('Password and confirmation must match').css('color', 'red');
})


function validPassword() {
    if ($('#password1Register').val() !== $('#password2Register').val())
        return false;

    // validate password
        
    return true;
}


function registerFieldsCheck(e) {
    e.preventDefault();

    if (!validPassword())
        return false;
    
    fetch('/accounts/register/', {
        method: 'POST',
        // headers: {
        //     'Content-type': 'application/json'
        // },
        // body: JSON.stringify({
        //     'email': ,
        //     'password1': ,
        //     'password2': ,
        // })
    })
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

$('.form').find('input, textarea').on('keyup blur focus', function (e) {
  
    var $this = $(this),
        label = $this.prev('label');

    if (e.type === 'keyup') {
            if ($this.val() === '') {
        label.removeClass('active highlight');
        } else {
        label.addClass('active highlight');
        }
    } else if (e.type === 'blur') {
        if( $this.val() === '' ) {
            label.removeClass('active highlight'); 
            } else {
            label.removeClass('highlight');   
            }   
    } else if (e.type === 'focus') {
    
    if( $this.val() === '' ) {
            label.removeClass('highlight'); 
            } 
    else if( $this.val() !== '' ) {
            label.addClass('highlight');
            }
    }

});

$('.tab a').on('click', function (e) {

    e.preventDefault();

    $(this).parent().addClass('active');
    $(this).parent().siblings().removeClass('active');

    target = $(this).attr('href');

    $('.tab-content > div').not(target).hide();

    $(target).fadeIn(600);

});
